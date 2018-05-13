#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "tree.h"
#include "proc-common.h"

void fork_procs(struct tree_node *t){
	/*
	 * Start
	 */
	
	int i;
	
	printf("PID = %ld, name %s, starting...\n", (long)getpid(), t->name);
	change_pname(t->name);
	
	if(t->nr_children == 0) {    //elegxos gia to an einai fullo
		
		raise(SIGSTOP);     //stelnei to paidi ston eauto tou minima oste na stamatisei thn leitourgia tou mexri na tou erthei shma
		printf("PID = %ld, name = %s is awake\n",(long)getpid(), t->name);
	}
	else{
		
		int status[t->nr_children];
		pid_t pid[t->nr_children];
		
		for (i=0; i<t->nr_children; i++){     //dimiourgia dentrou diergasion me anadromiko tropo
			
			pid[i]=fork();               //dimiourgia neas diergasias
			if (pid[i] < 0) {
				perror("proc: fork");
				exit(1);
			}
			if (pid[i] == 0) {          //tmima  pou mpainei to paidi
				/* Child */
				fork_procs(t->children+i);  //anadromikh klisi oste na sxhmatistei to dentro
				exit(1);
			}
			
			wait_for_ready_children(1);        //o kathe pateras perimenei na koimithoun ta paidia gia na sunexisei
			
		}
			
		raise(SIGSTOP);              //o arxikos pateras stamataei thn leitourgia tou mexri na tou erthei kapoio shma
		printf("PID = %ld, name = %s is awake\n", (long)getpid(), t->name);

		for(i=0 ; i<t->nr_children ; i++){        //o kathe pateras stelnei sta paidia tou shma na sunexisoun thn leitourgia tous
			kill(pid[i],SIGCONT);
			waitpid(pid[i], &status[i], 0);
			explain_wait_status(pid[i],status[i]);
		}
	}
	/*
	 * Exit
	 */
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

int main(int argc, char *argv[])
{
	pid_t pid;
	int status;
	struct tree_node *root;

	if (argc < 2){         // elegxos sostis eisagoghs orismaton
		fprintf(stderr, "Usage: %s <tree_file>\n", argv[0]);
		exit(1);
	}

		/* Read tree into memory */
	root = get_tree_from_file(argv[1]);

	/* Fork root of process tree */
	pid = fork();         //dimiourgia neas diergasias
	if (pid < 0) {        //elegxos
		perror("main: fork");
		exit(1);
	}
	if (pid == 0) {      //tmima paidiou
		/* Child */
		fork_procs(root);
		exit(1);
	}

	/*
	 * Father
	 */
	/* for ask2-signals */
	wait_for_ready_children(1);  //stamataei mexri na koimithoun ta paidia

	/* for ask2-{fork, tree} */
	/* sleep(SLEEP_TREE_SEC); */

	/* Print the process tree root at pid */
	show_pstree(pid);          //emfanish dentrou

	/* for ask2-signals */
	kill(pid, SIGCONT);       //shma oste na sinexisei to paidi

	/* Wait for the root of the process tree to terminate */
	wait(&status);            //perimenei oste na steilei kapoio minima o pateras
	explain_wait_status(pid, status);

	return 0;
}

