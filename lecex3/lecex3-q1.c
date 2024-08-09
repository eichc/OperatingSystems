#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <ctype.h>

int lecex3_q1_child(int pipefd) {
	//get key and size from pipe
	int key, size;
	read(pipefd, &key, sizeof(key));
	read(pipefd, &size, sizeof(size));

	//attach to shared memory
	int shmid = shmget(key, size, 0660);
	if (shmid == -1)
	{
		perror("shmget() failed");
	    return EXIT_FAILURE;
	}

	char * data = shmat(shmid, NULL, 0);
	if (data == (void *)-1)
	{
	     perror("shmat() failed");
	    return EXIT_FAILURE;
	}

	//access the memory and convert to uppercase
	for (int i = 0; i < size; i++) {
		*(data+i) = toupper(*(data+i));
	}

	// detach from the shared memory segment
  	int rc = shmdt(data);
  	if (rc == -1)
  	{
    	perror("shmdt() failed");
    	return EXIT_FAILURE;
  	}

	return EXIT_SUCCESS;
}