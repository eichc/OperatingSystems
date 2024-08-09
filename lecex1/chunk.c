#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

int main(int argc, char** argv)
{
	//check args
	if (argc != 3) {
		fprintf(stderr, "ERROR: Invalid arguments\n");
    	fprintf(stderr, "USAGE: a.out <n> <filename>\n");
    	return EXIT_FAILURE;
	}
	int n = atoi(*(argv+1));
	char * filename = *(argv+2);

	//open file
	int infile = open(filename, O_RDONLY);
  	if (infile == -1)
  	{
    	fprintf(stderr, "open() failed\n");
    	return EXIT_FAILURE;
  	}

  	//read file
  	char * buffer = malloc(n + 1);
  	int i = 0;
  	int len;
  	while ((len = read(infile, buffer, n)) == n) {
  		*(buffer+len) = '\0';
  		if (i > 0) {
  			printf("|");
  		}
  		printf("%s", buffer);
  		i = i + 1;
  		lseek(infile, n, SEEK_CUR);
  	}
  	printf("\n");

  	close(infile);
  	free(buffer);

	return EXIT_SUCCESS;
}