#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

extern int total_guesses;
extern int total_wins;
extern int total_losses;
extern char ** words;
char** dict;
pthread_mutex_t mutex;
int sizeof_words, games, listener, dict_size;
bool killsig;


int fill_dict(int fd) {
	char* buffer = calloc(7, sizeof(char));
	for (int i = 0; i < dict_size; i++) {
		if (read(fd, buffer, 6) == 0) {
			fprintf(stderr, "ERROR: Incorrect dictionary size\n");
			free(buffer);
			return EXIT_FAILURE;
		}
		*(buffer+5) = '\0';
		//convert to lowercase
		for (int j = 0; j < 5; j++) {
			*(buffer+j) = tolower(*(buffer+j));
		}
		*(dict+i) = calloc(6, sizeof(char));
		strcpy(*(dict+i), buffer);
	}
	free(buffer);
	return EXIT_SUCCESS;
}

void signal_handler(int sig) {
	killsig = true;
	close(listener);
}

bool validate_guess(char* guess) {
	for (int i = 0; i < dict_size; i++) {
		if (strcmp(guess, *(dict+i)) == 0) {
			return true;
		}
	}
	return false;
}

char* grade_guess(char* guess, char* secret) {
	char* result = calloc(6, sizeof(char));
	bool* matched = calloc(5, sizeof(bool));
	strcpy(result, "-----");

	//find correct letters
	for (int i = 0; i < 5; i++) {
		if (*(guess+i) == *(secret+i)) {
			*(result+i) = toupper(*(guess+i));
			*(matched+i) = true;
		}
	}

	//find letters in wrong place
	for (int i = 0; i < 5; i++) { //outer loop is for guess, inner loop for secret
		if (*(guess+i) == *(secret+i)) {
			continue;
		}
		for (int j = 0; j < 5; j++) {
			if (*(matched+j) == false && *(secret+j) == *(guess+i)) {
				*(result+i) = *(guess+i);
				*(matched+j) = true;
			}
		}
	}

	free(matched);
	return result;
}

void *threadMain(void *arg) {
	pthread_mutex_lock(&mutex);
    games++;
    pthread_mutex_unlock(&mutex);
	int newsd = *(int*)arg;
	bool win = false;
	char* guess = calloc(9, sizeof(char));
	char* packet = calloc(9, sizeof(char));
	char* accuracy;
	int n;
	int remaining = 6;
	int random = (rand() % dict_size);

	//get secret word
	char* secret = calloc(6, sizeof(char));
	strcpy(secret, *(dict+random));
	char* secret_caps = calloc(6, sizeof(char));
	strcpy(secret_caps, secret);
	for (int i = 0; i < 5; i++) {
		*(secret_caps+i) = toupper(*(secret_caps+i));
	}
	pthread_mutex_lock(&mutex);
	words = realloc(words, (sizeof_words+2) * sizeof(char*));
	*(words+sizeof_words) = calloc(6, sizeof(char));
	strcpy(*(words+sizeof_words), secret_caps);
	*(words+sizeof_words+1) = NULL;
	sizeof_words++;
	pthread_mutex_unlock(&mutex);
	free(secret_caps);


	//loop to receive each guess
	while (remaining > 0) {
		if (killsig) {
			break;
		}

		//read in guess
		printf("THREAD %lu: waiting for guess\n", pthread_self());
		n = recv(newsd, guess, 5, 0);
		if (n == -1) {
        	perror("ERROR: recv() failed\n");
            pthread_exit(NULL);
        } else if (n == 0) { //if client gave up
        	printf("THREAD %lu: client gave up; closing TCP connection...\n", pthread_self());
        	break;
        }
        *(guess+n) = '\0';
        for (int i = 0; i < 5; i++) {
			*(guess+i) = tolower(*(guess+i));
		}
		printf("THREAD %lu: rcvd guess: %s\n", pthread_self(), guess);

		//check if guess is in dictionary
		if (validate_guess(guess)) {
			pthread_mutex_lock(&mutex);
			total_guesses++;
			pthread_mutex_unlock(&mutex);
			remaining--;
			//determind accuracy of guess
			accuracy = grade_guess(guess, secret);
			if (remaining == 1) {
				printf("THREAD %lu: sending reply: %s (%d guess left)\n", pthread_self(), accuracy, remaining);
			} else {
				printf("THREAD %lu: sending reply: %s (%d guesses left)\n", pthread_self(), accuracy, remaining);
			}
			//build and send the packet
			*(packet) = 'Y';
			*(short*)(packet+1) = htons(remaining);
			strcpy(packet+3, accuracy);
			n = send(newsd, packet, 8, 0);
			if (n == -1) {
        		perror("ERROR: send() failed\n");
            	pthread_exit(NULL);
        	}
        	//check if the guess was correct
        	if (strcmp(guess, secret) == 0) {
        		printf("THREAD %lu: game over; word was %s!\n", pthread_self(), accuracy);
        		pthread_mutex_lock(&mutex);
        		total_wins++;
        		pthread_mutex_unlock(&mutex);
        		win = true;
        		free(accuracy);
        		break;
        	}
        	free(accuracy);
		} else {
			if (remaining == 1) {
				printf("THREAD %lu: invalid guess; sending reply: ????? (%d guess left)\n", pthread_self(), remaining);
			} else {
				printf("THREAD %lu: invalid guess; sending reply: ????? (%d guesses left)\n", pthread_self(), remaining);
			}
			//build and send the packet
			*(packet) = 'N';
			*(short*)(packet+1) = htons(remaining);
			strcpy(packet+3, "?????");
			n = send(newsd, packet, 8, 0);
			if (n == -1) {
        		perror("ERROR: send() failed\n");
            	pthread_exit(NULL);
        	}
		}
	}

	//check if loss
	if (!win && !killsig) {
		for (int i = 0; i < 5; i++) {
			*(secret+i) = toupper(*(secret+i));
		}
		printf("THREAD %lu: game over; word was %s!\n", pthread_self(), secret);
		pthread_mutex_lock(&mutex);
        total_losses++;
        pthread_mutex_unlock(&mutex);
	}

	close(newsd);
	free(guess);
	free(packet);
	free(secret);
	pthread_mutex_lock(&mutex);
    games--;
    pthread_mutex_unlock(&mutex);
	pthread_exit(NULL);
}

int wordle_server( int argc, char ** argv ) {
	setvbuf( stdout, NULL, _IONBF, 0 );
	//initialize command line vars
	if (argc != 5) {
		fprintf(stderr, "ERROR: Invalid argument(s)\nUSAGE: hw3.out <listener-port> <seed> <dictionary-filename> <num-words>\n");
		return EXIT_FAILURE;
	}
	int port = atoi(*(argv+1));
	int seed = atoi(*(argv+2));
	dict_size = atoi(*(argv+4));
	int fd = open(*(argv+3), O_RDONLY);
	if (fd == -1 || port < 0 || seed < 0 || dict_size < 1)
  	{
    	fprintf(stderr, "ERROR: Invalid argument(s)\nUSAGE: hw3.out <listener-port> <seed> <dictionary-filename> <num-words>\n");
    	return EXIT_FAILURE;
  	}
  	printf("MAIN: opened %s (%d words)\n", *(argv+3), dict_size);
  	srand(seed);
  	printf("MAIN: seeded pseudo-random number generator with %d\n", seed);

  	//read all words into the dictionary array
  	dict = calloc(dict_size, sizeof(char*));
  	if (fill_dict(fd) == EXIT_FAILURE) { return EXIT_FAILURE; }
  	close(fd);

  	//create listener socket
  	listener = socket(AF_INET, SOCK_STREAM, 0);
  	if (listener == -1) {
  		fprintf(stderr, "ERROR: socket() failed\n");
  		return EXIT_FAILURE;
  	}
  	struct sockaddr_in tcp_server;
  	tcp_server.sin_family = AF_INET;
  	tcp_server.sin_port = htons(port);
  	tcp_server.sin_addr.s_addr = htonl(INADDR_ANY);

  	//bind listener socket and listen for connection
  	if (bind(listener, (struct sockaddr*)&tcp_server, sizeof(tcp_server)) == -1) {
	    fprintf(stderr, "ERROR: bind() failed\n");
	    return EXIT_FAILURE;
  	}
  	if (listen(listener, 5) == -1) {
	    fprintf(stderr, "ERROR: listen() failed\n");
	    return EXIT_FAILURE;
	}
	printf("MAIN: Wordle server listening on port {%d}\n", port);

	//set up signal handler
  	killsig = false;
  	//signal(SIGINT, SIG_IGN);
  	//signal(SIGTERM, SIG_IGN);
  	signal(SIGUSR2, SIG_IGN);
  	signal(SIGUSR1, signal_handler);

	pthread_mutex_init(&(mutex), NULL);
	sizeof_words = 0;
	games = 0;
	while (1) {
		//accept client connection
		struct sockaddr_in remote_client;
	    int addrlen = sizeof(remote_client);
	    int newsd = accept(listener, (struct sockaddr*)&remote_client, (socklen_t*)&addrlen);
	    if (killsig) {
			break;
		}
	    if (newsd == -1) { 
	    	fprintf(stderr, "ERROR: accept() failed\n"); 
	    	return EXIT_FAILURE; 
	    }
	    printf("MAIN: rcvd incoming connection request\n");

	    //create thread to run this instance
	    pthread_t tid;
	    if (pthread_create(&tid, NULL, threadMain, &newsd) != 0) {
	    	perror("ERROR: Create thread failed\n");
	    	return EXIT_FAILURE;
	    }
	    if (pthread_detach(tid) != 0) {
	    	perror("ERROR: Detach thread failed\n");
	    	return EXIT_FAILURE;
	    }
	}

	//wait for all games to end
	while (games != 0) {;}


	//free memory and exit
	for (int i = 0; i < dict_size; i++) {
		free(*(dict+i));
	}
	free(dict);
	printf("MAIN: SIGUSR1 rcvd; Wordle server shutting down...\n");
  	return EXIT_SUCCESS;
}