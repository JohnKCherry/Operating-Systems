#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

#include "tree.h"
#include "proc-common.h"

#define SLEEP_TREE_SEC  3
#define SLEEP_PROC_SEC 10

void fork_procs(struct tree_node *t, int parent_pipe[]) {
	int i;
	
	printf("PID = %ld, name %s, starting...\n",	(long)getpid(), t->name);
	change_pname(t->name);
	
	if (t->nr_children == 0) {
		int leaf_value;
		
		sleep(SLEEP_PROC_SEC);
		close(parent_pipe[0]);
		leaf_value = atoi(t->name);
		ssize_t wrcnt=write(parent_pipe[1], &leaf_value, sizeof(int));
		if (wrcnt<0){
			perror("Write");
			exit(1);
		}
		
		printf("%s with PID = %ld is ready to terminate...\n%s: Exiting...\n", t->name, (long)getpid(), t->name);
		exit(0);
	}
	else {
		int numbers[2];
		int my_pipe[2];
		if (pipe(my_pipe)){
			perror("Failed pipe");
		}
		
		int status;
		pid_t pid;
		
		for(i=0; i<2; i++){
			pid = fork();
			if (pid < 0) {
				perror("proc: fork");
				exit(1);
			}
			if (pid == 0) {
				/* Child */
				fork_procs(t->children+i, my_pipe);
				exit(1);
			}
		}
		
		close(my_pipe[1]);
		for (i=0; i<2; i++) {
			ssize_t rcnt=read(my_pipe[0], numbers+i, sizeof(int));
			if (rcnt<0){
				perror("Read");
			exit(1);
		}
		}
		
		int result;
		if (!strcmp(t->name,"+")) result = numbers[0]+numbers[1];
		else result = numbers[0]*numbers[1];
		
		printf("%d %s %d = %d\n", numbers[0], t->name, numbers[1], result);
		
		ssize_t wrcnt=write(parent_pipe[1], &result, sizeof(int));
		if (wrcnt<0){
			perror("Write");
			exit(1);
		}
		
		for (i=0; i<2; i++) {
			wait(&status);
			explain_wait_status(pid, status);
		}
	}
	printf("%s with PID = %ld is ready to terminate...\n%s: Exiting...\n", t->name, (long)getpid(), t->name);
	exit(0);
}

/*
 * The initial process forks the root of the process tree,
 * waits for the process tree to be completely created,
 * then takes a photo of it using show_pstree().
 *
 * How to wait for the process tree to be ready?
 * In ask2-{fork, tree}:
 *      wait for a few seconds, hope for the best.
 * In ask2-signals:
 *      use wait_for_ready_children() to wait until
 *      the first process raises SIGSTOP.
 */

int main(int argc, char *argv[]){
	pid_t pid;
	int status, pipefd[2];
	struct tree_node *root;

	if (argc < 2){
		fprintf(stderr, "Usage: %s <tree_file>\n", argv[0]);
		exit(1);
	}

	/* Read tree into memory */
	root = get_tree_from_file(argv[1]);
	//print_tree(root);
	
	if (pipe(pipefd)) {
			perror("Failed pipe");
	}
	
	/* Fork root of process tree */
	pid = fork();
	if (pid < 0) {
		perror("main: fork");
		exit(1);
	}
	if (pid == 0) {
		/* Child */
		fork_procs(root, pipefd);
		exit(1);
	}

	/*
	 * Father
	 */
	/* for ask2-signals */
	//wait_for_ready_children(1);

	/* for ask2-{fork, tree} */
	 sleep(SLEEP_TREE_SEC); 

	/* Print the process tree root at pid */
	show_pstree(pid);
	
	int numbers[2], result;
	ssize_t rcnt=read(pipefd[0], numbers, 2*sizeof(int));
	if (rcnt<0){
			perror("Read");
			exit(1);
		}
	result = numbers[0]+numbers[1];
	
	/* Wait for the root of the process tree to terminate */
	waitpid(pid, &status, 0);	
	explain_wait_status(pid, status);
	
	printf("%d\n", result);
	
	return 0;
}
