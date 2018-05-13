#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <assert.h>

#include <sys/wait.h>
#include <sys/types.h>

#include "proc-common.h"
#include "request.h"
#include "list.h"

processT curr;
processListT procList;


/* Compile-time parameters. */
#define SCHED_TQ_SEC 2                /* time quantum */
#define TASK_NAME_SZ 60               /* maximum size for a task's name */


/*
 * SIGALRM handler
 */
static void
sigalrm_handler(int signum)
{
	if (signum != SIGALRM) {
		fprintf(stderr, "Internal error: Called for signum %d, not SIGALRM\n",
			signum);
		exit(1);
	}
	fprintf(stderr,"Alarm!!! -.- \n");
	kill(curr->pPid, SIGSTOP);
}

/*
 * SIGCHLD handler
 */
static void sigchld_handler(int signum)
{
	pid_t p;
	int status;

	if (signum != SIGCHLD) {
		fprintf(stderr, "Internal error: Called for signum %d, not SIGCHLD\n",
			signum);
		exit(1);
	}

	/*
	 * Something has happened to one of the children.
	 * We use waitpid() with the WUNTRACED flag, instead of wait(), because
	 * SIGCHLD may have been received for a stopped, not dead child.
	 *
	 * A single SIGCHLD may be received if many processes die at the same time.
	 * We use waitpid() with the WNOHANG flag in a loop, to make sure all
	 * children are taken care of before leaving the handler.
	 */
	for(;;) {
		p = waitpid(-1, &status, WUNTRACED | WNOHANG);
		if (p < 0) {
			perror("waitpid");
			exit(1);
		}

		if (p == 0)
			break;

		explain_wait_status(p, status);

		if (WIFEXITED(status) || WIFSIGNALED(status)) {
		/* A child has died */
			struct process* temp = curr;
			curr = curr->next;
			fprintf(stderr,"Proccess with pid=%ld terminated...\n", (long int)temp->pPid);
			removeFromList(procList, temp);
			if(procList->head == NULL) {
				printf("All children finished. Exiting...\n");
				exit(EXIT_SUCCESS);
			}
			if(curr == NULL)
					curr = procList->head;

			/* start alarm */
			if (alarm(SCHED_TQ_SEC) < 0) {
				perror("alarm");
				exit(1);
			}
			
			fprintf(stderr,"Proccess with pid=%ld is about to begin...\n", (long int)curr->pPid);
			kill(curr->pPid, SIGCONT);
		}

		if (WIFSTOPPED(status)) {
			/* A child has stopped due to SIGSTOP/SIGTSTP, etc... */
			curr = curr->next;
			if (curr == NULL)
				curr = procList->head;

			/* start alarm */
			if (alarm(SCHED_TQ_SEC) < 0) {
				perror("alarm");
				exit(1);
			}
			fprintf(stderr,"Proccess with pid=%ld is about to begin...\n", (long int)curr->pPid);
			kill(curr->pPid, SIGCONT);
		}
	}
}

/* Install two signal handlers.
 * One for SIGCHLD, one for SIGALRM.
 * Make sure both signals are masked when one of them is running.
 */
static void
install_signal_handlers(void)
{
	sigset_t sigset;
	struct sigaction sa;

	sa.sa_handler = sigchld_handler;
	sa.sa_flags = SA_RESTART;
	sigemptyset(&sigset);
	sigaddset(&sigset, SIGCHLD);
	sigaddset(&sigset, SIGALRM);
	sa.sa_mask = sigset;
	if (sigaction(SIGCHLD, &sa, NULL) < 0) {
		perror("sigaction: sigchld");
		exit(1);
	}

	sa.sa_handler = sigalrm_handler;
	if (sigaction(SIGALRM, &sa, NULL) < 0) {
		perror("sigaction: sigalrm");
		exit(1);
	}

	/*
	 * Ignore SIGPIPE, so that write()s to pipes
	 * with no reader do not result in us being killed,
	 * and write() returns EPIPE instead.
	 */
	if (signal(SIGPIPE, SIG_IGN) < 0) {
		perror("signal: sigpipe");
		exit(1);
	}
}

void executeProg(processT proc){
	
	char *newargv[] = { proc->execName, NULL, NULL, NULL };
	char *newenviron[] = { NULL };
	
	execve(proc->execName, newargv, newenviron);
	
	perror("Execve:");
	exit(-1);
}

int main(int argc, char *argv[])
{
	int nproc;
	pid_t pid;
	int i;
	
	/*
	 * For each of argv[1] to argv[argc - 1],
	 * create a new child process, add it to the process list.
	 */
	 
	procList = initList();
	nproc = argc - 1; /* number of proccesses goes here */
	
	for(i=1; i<argc; i++){
		curr = initProc(argv[i], i);
		addToList(procList,curr);
	}
	
	curr = procList->head;
	for(i=0; i<nproc; i++){
		pid = fork();
		if(pid < 0){
			perror("");
			exit(-1);
		}
		if(pid == 0){
			fprintf(stderr,"A new proccess is created with pid=%ld \n",(long int)getpid());
			raise(SIGSTOP);
			executeProg(curr);
		}
		if(pid > 0){
			curr->pPid = pid;
			curr = curr->next;
		}
	}
	
	/* Wait for all children to raise SIGSTOP before exec()ing. */
	wait_for_ready_children(nproc);

	/* Install SIGALRM and SIGCHLD handlers. */
	install_signal_handlers();

	if (nproc == 0) {
		fprintf(stderr, "Scheduler: No tasks. Exiting...\n");
		exit(1);
	}
	
	curr = procList->head;
	
	if (alarm(SCHED_TQ_SEC) < 0) {
		perror("alarm");
		exit(-1);
	}
	
	fprintf(stderr,"Proccess with pid=%ld is about to begin...\n", (long int)curr->pPid);
	kill(curr->pPid, SIGCONT);

	/* loop forever  until we exit from inside a signal handler. */
	while (pause())
		;

	/* Unreachable */
	fprintf(stderr, "Internal error: Reached unreachable point\n");
	return 1;
}
