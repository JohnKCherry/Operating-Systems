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

void fork_procs(struct tree_node *t, int parentPipe[]) {
	int i;

	printf("PID = %ld, name %s, starting...\n",	(long)getpid(), t->name);
	change_pname(t->name);

	if (t->nr_children == 0) {      //tmima pou mpainoun ta fulla
		int value;

		sleep(SLEEP_PROC_SEC);   //koimountai oste na prolabei na dimiourgithei to dentro
		if (close(parentPipe[0]) < 0){  
			perror("Close");
			exit(1);
		}
		value = atoi(t->name);    //metatroph xarakthra se arihtmo
		if (write(parentPipe[1], &value, sizeof(int)) < 0){    //grafei sto akro (1) ths pipe kai elegxei an graftike 
			perror("Write");
			exit(1);
		}

		printf("%s with PID = %ld is ready to terminate...\n%s: Exiting...\n", t->name, (long)getpid(), t->name);
		exit(0);
	}
	else {
		int numbers[2];  //dilosh ton duo arithmon ton duo fullon
		int myPipe[2];   //dilosh ton duo deikton sthn pipe. 1->gia eggrafh , 0-> gia anagnosh
		if (pipe(myPipe)){
			perror("Failed pipe");
		}

		int status;
		pid_t pid;

		for(i=0; i<2; i++){        //dimiourgia tou dentrou
			pid = fork();
			if (pid < 0) {
				perror("proc: fork");
				exit(1);
			}
			if (pid == 0) {
				/* Child */
				fork_procs(t->children+i, myPipe);
				exit(1);
			}
		}

		if (close(myPipe[1]) < 0){  
			perror("Close");
			exit(1);
		}
		for (i=0; i<2; i++) {
			if (read(myPipe[0], numbers+i, sizeof(int)) < 0){   // diabazei tous arithous ton duo fullon
				perror("Read");
				exit(1);
			}
		}

		int result;
		if (!strcmp(t->name,"+")) result = numbers[0]+numbers[1];  //elegxei thn praksh apo ton patera kai ektelei thn antistoixh
		else result = numbers[0]*numbers[1];                       //praksh


		if (write(parentPipe[1], &result, sizeof(int))<0){         //grapse to apotelesma sto pipe      
			perror("Write");
			exit(1);
		}

		for (i=0; i<2; i++) {                                      //sunexisei o pateraas thn ektelesh tou
			waitpid(pid, &status, 0);
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
	int status, pipe_fd[2];
	struct tree_node *root;

	if (argc < 2){                 //elegxos orismaton
		fprintf(stderr, "Usage: %s <tree_file>\n", argv[0]);
		exit(1);
	}

	/* Read tree into memory */
	root = get_tree_from_file(argv[1]);
	//print_tree(root);

	if (pipe(pipe_fd)) {           //elegxos gia to an h pipe dimiourgithike sosta
		perror("Failed pipe");
	}

	/* Fork root of process tree */
	pid = fork();                  //dimiourgia neas diergasias
	if (pid < 0) {
		perror("main: fork");
		exit(1);
	}
	if (pid == 0) {                //tmima paidiou
		/* Child */
		fork_procs(root, pipe_fd);
		exit(1);
	}

	/*
	 * Father
	 */
	/* for ask2-signals */
	//wait_for_ready_children(1);

	/* for ask2-{fork, tree} */
	sleep(SLEEP_TREE_SEC);        //koimatai o pateras oste na dimioutgithe to dentro kai na graftei to apotelesma

	/* Print the process tree root at pid */
	show_pstree(pid);             //emfanisi tou dentrou

	int result;
	if (read(pipe_fd[0], &result, sizeof(int)) < 0){    //diabasma kai ekxorisi tou apotelesmatos sto result
		perror("Read");
		exit(1);
	}

	/* Wait for the root of the process tree to terminate */
	waitpid(pid, &status, 0);	
	explain_wait_status(pid, status);

	printf("\nThe result is %d\n", result);

	return 0;
}
