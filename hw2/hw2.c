#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <stdbool.h>

#ifdef DEBUG_MODE
void print_board(char** board, int m, int n) {
	for (int i = 0; i < m; i++) {
		for (int j = 0; j < n; j++) {
			if (*(*(board+i)+j) == 0) {
				printf("-");
			} else {
				printf("%c", *(*(board+i)+j));
			}
		}
		printf("\n");
	}
}
#endif

/* Determine if a Queen can safely be added to (x, y) */
bool is_safe_move(char** board, int m, int n, int y, int x) {
	int left = x-1;
	int right = x+1;
	for (int i = 1; i <= y; i++) {
		if (*(*(board+y-i)+x) == 'Q') { //check directly above
			return false;
		} else if (left >= 0 && *(*(board+y-i)+left) == 'Q') { //check left diagonal
			return false;
		} else if (right < n && *(*(board+y-i)+right) == 'Q') { //check right diagonal
			return false;
		}
		left--;
		right++;
	}
	return true;
}

/* This function blocks the current process until the child has finished. */
void block_on_waitpid(int pid) {
	int status;
	pid_t child_pid = waitpid(pid, &status, 0);
	if (WIFSIGNALED(status)) {
		fprintf(stderr, "%d: ERROR: Child process %d terminated abnormally\n", getpid(), child_pid);
		abort();
	}
}

/* This function ensures all child processes have finished before the parent returns. */
void parent_process(int children) {
	while (children > 0) {
		block_on_waitpid(-1);
		children--;
	}
}

/* Find all valid moves from the current board and create necessary child processes. Also report if any solutions or dead ends were found. */
int child_process(char** board, int* pipefd, int m, int n, int row) {
	#ifdef DEBUG_MODE
		print_board(board, m, n);
	#endif

	if (row == m) { //solution is found
		#ifndef QUIET
			printf("%d: found a solution; notifying top-level parent\n", getpid());
		#endif

		//write the number of queens as chars
		unsigned char num = (unsigned char) row;
		write(*(pipefd+1), &num, sizeof(num));
		close(*(pipefd+1));
		
		for (int i = 0; i < m; i++) {
			free(*(board+i));
		}
		free(board);
		free(pipefd);
		return EXIT_SUCCESS;
	}

	//initialize current row of board
	*(board+row) = calloc(n, sizeof(char));

	//count number of moves
	bool* valid = calloc(n, sizeof(bool));
	int valid_moves = 0;
	for (int i = 0; i < n; i++) {
		if (is_safe_move(board, m, n, row, i)) {
			valid_moves++;
			*(valid+i) = true;
		}
	}
	//print output based on number of moves
	#ifndef QUIET
		if (valid_moves == 0) { //dead end
			printf("%d: dead end at row #%d; notifying top-level parent\n", getpid(), row);
		} else if (valid_moves == 1) {
			printf("%d: 1 possible move at row #%d; creating 1 child process...\n", getpid(), row);
		} else {
			printf("%d: %d possible moves at row #%d; creating %d child processes...\n", getpid(), valid_moves, row, valid_moves);
		}
	#endif

	//write result to pipe if dead end
	if (valid_moves == 0) {
		unsigned char num = (unsigned char) row;
		write(*(pipefd+1), &num, sizeof(num));
	}

	//create child processes
	for (int i = 0; i < n; i++) {
		#ifdef DEBUG_MODE
			usleep(10);
		#endif
		if (*(valid+i) == false) { //only create process for valid moves
			continue;
		}
		pid_t p = fork();
		if (p == -1) {
			fprintf(stderr, "%d: ERROR: fork() failed\n", getpid());
			abort();
		}
		if (p == 0) { //child
			*(*(board+row)+i) = 'Q';
			free(valid);
			return child_process(board, pipefd, m, n, row+1);
		} else {
			#ifdef NO_PARALLEL
			    block_on_waitpid(p);
			#endif
		}
	}
	close(*(pipefd+1));

	//wait for all child processes to finish before returning
	#ifndef NO_PARALLEL
	    int children = valid_moves;
	    parent_process(children);
	#endif

	free(valid);
	for (int i = 0; i <= row; i++) {
		free(*(board+i));
	}
	free(board);
	free(pipefd);
	return EXIT_SUCCESS;
}

int main(int argc, char** argv) {
	setvbuf(stdout, NULL, _IONBF, 0);
	//check command line args
	if (argc != 3) {
		fprintf(stderr, "ERROR: Invalid argument(s)\nUSAGE: hw2.out <m> <n>\n");
		return EXIT_FAILURE;
	}
	int m = atoi(*(argv+1));
	int n = atoi(*(argv+2));
	if (m < 1 || n < 1) {
		fprintf(stderr, "ERROR: Invalid argument(s)\nUSAGE: hw2.out <m> <n>\n");
		return EXIT_FAILURE;
	}

	//swap if n < m
	if (n < m) {
		int temp = n;
		n = m;
		m = temp;
	}

	//create initial board and pipe
	char** board = calloc(m, sizeof(char*));
	*(board) = calloc(n, sizeof(char));

	int* pipefd = calloc(2, sizeof(int));
	int rc = pipe(pipefd);
	if (rc == -1) {
		fprintf(stderr, "ERROR: pipe() failed\n");
		abort();
	}
	#ifdef DEBUG
	fcntl(*(pipefd+1), F_SETPIPE_SZ, 262144);
	int s1 = fcntl(*(pipefd+1), F_GETPIPE_SZ);
	int s2 = fcntl(*pipefd, F_GETPIPE_SZ);
	printf("Pipe is %d, %d\n", s1, s2);
	#endif

	//start fork tree
	pid_t root_pid = getpid();
	printf("%d: solving the Abridged (m,n)-Queens problem for %dx%d board\n", root_pid, m, n);
	if (n == 1) {
		printf("%d: 1 possible move at row #0; creating 1 child process...\n", root_pid);
	} else {
		printf("%d: %d possible moves at row #0; creating %d child processes...\n", root_pid, n, n);
	}
	for (int i = 0; i < n; i++) {
		#ifdef DEBUG_MODE
			usleep(10);
		#endif
		pid_t p = fork();
		if (p == -1) {
			fprintf(stderr, "ERROR: fork() failed\n");
			abort();
		}
		if (p == 0) { //child
			close(*pipefd);
			*((*board)+i) = 'Q';
			return child_process(board, pipefd, m, n, 1);
		} else { //parent
			#ifdef NO_PARALLEL
			    block_on_waitpid(p);
			#endif
		}
	}
	close(*(pipefd+1));

	//wait for all child processes to finish before returning
	#ifndef NO_PARALLEL
		int children = m;
	    parent_process(children);
	#endif

	//count solutions
	int* solutions = calloc(m+1, sizeof(int));
	unsigned char sol;
	int int_sol;
	while (read(*pipefd, &sol, sizeof(sol))) {
		int_sol = (int) sol;
		*(solutions+int_sol) += 1;
	}
	close(*pipefd);

	//print results
	printf("%d: search complete\n", root_pid);
	for (int i = 1; i < m+1; i++) {
		printf("%d: number of %d-Queen end-states: %d\n", root_pid, i, *(solutions+i));
	}

	//free memory
	free(solutions);
	free(pipefd);
	for (int i = 0; i < m; i++) {
		free(*(board+i));
	}
	free(board);


	return EXIT_SUCCESS;
}