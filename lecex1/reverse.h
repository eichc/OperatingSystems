#include <stdlib.h>
#include <stdio.h>
#include <string.h>

char* reverse(char* s)
{
	int i, len = strlen(s);
	char* buffer = malloc(len + 1);

	//fill buffer with reversed string
	for (i = 0; i < len; i++) {
		*(buffer + i) = *(s+ len-i-1);
	}
	*(buffer + len) = '\0';

	//transfer reversed string to s
	for (i = 0; i <= len; i++) {
		*(s+i) = *(buffer+i);
	}
	free(buffer);

	return s;
}