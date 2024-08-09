#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>

#define MAX_WORD 128


/**
 * Determine the hash code for a word by adding ASCII value of each char
 */
int hash(char* word, int cacheSize) {
	int sum = 0;
	for (int i = 0; i < strlen(word); i++) {
		sum += (int)*(word+i);
	}
	return sum % cacheSize;
}

/**
 * Add a word to the cache and print the result
 */
void addToCache(char* word, int len, char** cache, int cacheSize) {
	int hashNum = hash(word, cacheSize);
	printf("Word \"%s\" ==> %d ", word, hashNum);
	if (*(cache+hashNum) == NULL) {
		*(cache+hashNum) = calloc(len+1, sizeof(char));
		printf("(calloc)\n");
	} else if (strlen(*(cache+hashNum)) != len) {
		*(cache+hashNum) = realloc(*(cache+hashNum), sizeof(char)*len+1);
		printf("(realloc)\n");
	} else {
		printf("(nop)\n");
	}
	strcpy(*(cache+hashNum), word);
}

/**
 * Read an input file, extract its words, and put the words into a cache
 */
int handle_file(char* filename, char** cache, int cacheSize) {
	int fd = open(filename, O_RDONLY);
  	if (fd == -1)
  	{
    	fprintf(stderr, "ERROR: open(%s) failed.\n", filename);
    	return -1;
  	}

  	//read file into buffer and extract words
  	char* buffer = calloc(MAX_WORD + 1, sizeof(char));
  	char* currWord = calloc(1, sizeof(char));
  	int buffLen = 0, wordLen = 0;
  	while ((buffLen = (read(fd, buffer, MAX_WORD)))) {
  		//looping through each character
  		for (int i = 0; i < buffLen; i++) {
	  		if (isalnum(*(buffer+i))) { //if still reading word
	  			wordLen++;
	  			currWord = realloc(currWord, sizeof(char)*wordLen+1);
	  			*(currWord+wordLen-1) = *(buffer+i);
	  		} else if (wordLen > 2) { //if end of word
	  			//add word to cache
	  			*(currWord+wordLen) = '\0';
	  			addToCache(currWord, wordLen, cache, cacheSize);
	  			wordLen = 0;
	  			currWord = realloc(currWord, sizeof(char));
	  		} else { //if not a word
	  			wordLen = 0;
	  		}
	  	}
  	}
  	if (wordLen > 0) { //if file ends in a word
  		*(currWord+wordLen) = '\0';
		addToCache(currWord, wordLen, cache, cacheSize);
	}

  	//free memory and close file
  	free(currWord);
  	free(buffer);
  	close(fd);
  	return 0;
}


int main(int argc, char** argv) {
	//check args
	if (argc < 3) {
		fprintf(stderr, "ERROR: Incorrect number of arguments.\n");
    	return EXIT_FAILURE;
	}

	//create initial cache
	int size = atoi(*(argv+1));
	if (size < 1) {
		fprintf(stderr, "ERROR: Cache must be a positive integer.\n");
    	return EXIT_FAILURE;
	}
	char** cache = calloc(size, sizeof(char*));

	//go through each input file
	for (int i = 2; i < argc; i++) {
		if (handle_file(*(argv+i), cache, size) == -1) {
			return EXIT_FAILURE;
		}
	}

	//print cache and free memory
	printf("\nCache:\n");
	for (int i = 0; i < size; i++) {
		if (*(cache+i) == NULL) {
			continue;
		}
		printf("%c%d%c ==> \"%s\"\n", 91, i, 93, *(cache+i));
		free(*(cache+i));
	}
	free(cache);

	return EXIT_SUCCESS;
}