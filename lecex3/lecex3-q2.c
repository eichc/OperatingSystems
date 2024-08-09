#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
extern void * copy_file( void * arg );

int main(int argc, char** argv) {
	int numThreads = argc-1;
	pthread_t* tids = (pthread_t *) malloc(sizeof(pthread_t)*numThreads);

	for (int i = 0; i < numThreads; i++) {
		printf("MAIN: Creating thread to copy \"%s\"\n", *(argv+i+1));
		pthread_create((tids+i), NULL, copy_file, (void *) *(argv+i+1));
	}
	int* retval;
	int total;
	for (int i = 0; i < numThreads; i++) {
		pthread_join(*(tids+i), (void**)&retval);
		printf("MAIN: Thread completed copying %d bytes for \"%s\"\n", *retval, *(argv+i+1));
		total += (*retval);
	}
	if (numThreads == 1) {
		printf("MAIN: Successfully copied %d bytes via %d child thread\n", total, numThreads);
	} else {
		printf("MAIN: Successfully copied %d bytes via %d child threads\n", total, numThreads);
	}

	free(retval);
	free(tids);
	return EXIT_SUCCESS;
}