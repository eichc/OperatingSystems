#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>

int lecex2_child( unsigned int n ) {
  if (n < 1) {
    fprintf(stderr, "ERROR: n must be positive.\n");
    abort();
  }

  //open file
  int fd = open("lecex2.txt", O_RDONLY);
  if (fd == -1)
    {
      fprintf(stderr, "ERROR: open(lecex2.txt) failed.\n");
      abort();
    }

    //read file
    char* buffer = calloc(n+1, sizeof(char));
    int bufLen = read(fd, buffer, n);
    *(buffer+bufLen) = '\0';
    if (bufLen < n) { //file was too short
      fprintf(stderr, "ERROR: input file was shorter than %d bytes.\n", n);
      abort();
    }

    char ret = *(buffer+n-1);
    free(buffer);
    return (int) ret;
}

int lecex2_parent() {
  //wait for child
  int status;
  waitpid( -1, &status, 0 );

  if (WIFSIGNALED(status)) {
    printf("PARENT: child process terminated abnormally!\n");
    return EXIT_FAILURE;
  }

  int exit_status = WEXITSTATUS(status);
  printf("PARENT: child process returned '%c'\n", exit_status);
  return EXIT_SUCCESS;
}