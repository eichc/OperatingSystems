#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>

int main(int argc, char** argv) {
	//create child process
	pid_t pid = fork();
	if (pid == -1) {
	    fprintf(stderr, "PARENT: fork() failed\n");
	    return EXIT_FAILURE;
	}

	if (pid == 0) { //child

		pid_t p = fork();
		if (p == -1) {
			fprintf(stderr, "CHILD: fork() failed\n");
			abort();
		}

		if (p == 0) { //grandchild
			//open file
			int fd = open(*(argv+1), O_RDONLY);
			if (fd == -1) {
				fprintf(stderr, "GRANDCHILD: open(%s) failed\n", *(argv+1));
				return -1;
			}

			//read file
			int proper = 0, open = 0, len;
			char* buffer = calloc(129, sizeof(char));
			while ((len = read(fd, buffer, 128))) {
				*(buffer+len) = '\0';
				for (int i = 0; i < len; i++) {
					if (*(buffer+i) == '(') {
						open++;
					} else if (*(buffer+i) == ')' && open > 0) {
						proper++;
						open--;
					}
				}
			}
			if (proper > 127) {
				fprintf(stderr, "GRANDCHILD: unable to count more than 127 pairs of parentheses\n");
				free(buffer);
				return -1;
			} else if (proper == 1) {
				printf("GRANDCHILD: counted %d properly nested pair of parentheses\n", proper);
			} else {
				printf("GRANDCHILD: counted %d properly nested pairs of parentheses\n", proper);
			}
			free(buffer);
			return proper;

		} else { //child
			int status;
			waitpid(p, &status, 0);
    		int exit_status = WEXITSTATUS(status);

    		//check for grandchild error
    		if (exit_status == 255) {
	    		fprintf(stderr, "CHILD: rcvd -1 (error)\n");
	    		return -1;
	    	}

	    	printf("CHILD: doubled %d; returning %d\n", exit_status, exit_status*2);
	    	return exit_status*2;
		}

	} else { //parent
		int status;
    	waitpid(pid, &status, 0);
    	int exit_status = WEXITSTATUS(status);

    	//check for child error
    	if (exit_status == 255) {
    		fprintf(stderr, "PARENT: rcvd -1 (error)\n");
    		return EXIT_FAILURE;
    	}

    	printf("PARENT: there are %d properly nested parentheses\n", exit_status);
	}

	return EXIT_SUCCESS;
}