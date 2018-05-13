#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "tree.h"
#include "proc-common.h"

#define SLEEP_TREE_SEC  3
#define SLEEP_PROC_SEC 10

void fork_procs(struct tree_node *t){
	
	int i;
	
	printf("PID = %ld, name %s, starting...\n",	(long)getpid(), t->name);
	change_pname(t->name);                //dinoume sthndiergasia to onoma pou exei sto tree
	
	if (t->nr_children == 0){             //elegxos gia to an einai fulla oste na koimithoun
		sleep(SLEEP_PROC_SEC);
		printf("%s with PID = %ld is ready to terminate...\n%s: Exiting...\n", t->name, (long)getpid(), t->name);
		exit(0);
	}
	else{
		
		int status;
		pid_t pid[t->nr_children];
		
		for(i=0; i<t->nr_children; i++){       //kodikas pou ftiaxnei to tree diergasion
			
			pid[i] = fork();               //dimiourgia neas diergasias
			if (pid[i] < 0) {              //elegxos
				perror("proc: fork"); 
				exit(1);
			}
			if (pid[i] == 0) {             //tmimi kodika sto opoio tha bei to paidi
				/* Child */
				fork_procs(t->children+i); //kalei anadromika thn fork_procs gia na dimiourgithei olo to dentro
				exit(1);
			}
			
		}
		for (i=0; i<t->nr_children; i++){         //ksipnima ton pateradon meta thn dimiourgia kai to thanato ton paidion tous
			waitpid(pid[i], &status, 0);
			explain_wait_status(pid[i], status); //eksigisi tou logou thanatou
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
	int status;
	struct tree_node *root;

	if (argc < 2){
		fprintf(stderr, "Usage: %s <tree_file>\n", argv[0]);
		exit(1);
	}

	/* Read tree into memory */
	root = get_tree_from_file(argv[1]);    //to tree ths eidodou

	/* Fork root of process tree */
	pid = fork();                          //dimiourgia neas diergasias
	if (pid < 0) {                         //elegxos ths dimiourgias ths
		perror("main: fork");
		exit(1);
	}
	if (pid == 0) {                        //edo mpaienito paidi afou epistrefei me pid=0
		/* Child */
		fork_procs(root);
		exit(1);
	}

	/*
	 * Father
	 */
	/* for ask2-signals */
	//wait_for_ready_children(1);

	/* for ask2-{fork, tree} */
	 sleep(SLEEP_TREE_SEC);                  //koimatai o pateras oste na dimiourgithoun ta paidia tou

	/* Print the process tree root at pid */
	show_pstree(pid);                        //emfanisi tou dentrou diergasion

	/* Wait for the root of the process tree to terminate */
	waitpid(pid, &status, 0);        	//o pateras (to progrsmma) perimenei na pethanei to paidi tou gia na sunexisei
	explain_wait_status(pid, status);       //eksigisi thanatou
	
	return 0;
}
