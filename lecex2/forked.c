/* forked.c */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>

/* implement these functions in lecex2-q1.c */
int lecex2_child( unsigned int n );
int lecex2_parent();

int main()
{
  unsigned int n = 7;  /* or some other non-negative value ... */
  int rc;

  /* create a new (child) process */
  pid_t p = fork();

  if ( p == -1 )
  {
    perror( "fork() failed" );
    return EXIT_FAILURE;
  }

  if ( p == 0 )
  {
    rc = lecex2_child( n );
  }
  else /* p > 0 */
  {
    rc = lecex2_parent();
  }

  return rc;
}