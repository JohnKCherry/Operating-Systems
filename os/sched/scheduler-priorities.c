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

/* Compile-time parameters. */
#define SCHED_TQ_SEC 2                /* time quantum */
#define TASK_NAME_SZ 60               /* maximum size for a task's name */
#define SHELL_EXECUTABLE_NAME "shell" /* executable for shell */

processListT procList;
processT curr;

int exceptions [2] = {0,0};
pid_t  exceptionsID [2];

void executeProg(processT proc){
	
	char *newargv[] = { proc->execName, NULL, NULL, NULL };
	char *newenviron[] = { NULL };
	
	execve(proc->execName, newargv, newenviron);
	
	perror("Execve:");
	exit(-1);
}

/* Print a list of all tasks currently being scheduled.  */
static void
sched_print_tasks(void)
{
	processT temp = procList->head;
	
	while(temp != NULL) {
		if (temp == curr) {
			printf("Currently running: \n");
		}

		printf("process ID: %d, "
			   "process PID: %ld, "
		   	   "executable name: %s"
		   	   "priority: %d\n",
			   temp->pNumber, (long)temp->pPid, temp->execName, temp->priority);

			   temp = temp->next;
	}
}

/* Send SIGKILL to a task determined by the value of its
 * scheduler-specific id.
 */
static int
sched_kill_task_by_id(int id)
{
	int flag = 0;
	processT temp = findProcID(procList, id, &flag);
	if (flag) {
		kill(temp->pPid, SIGKILL);
		fprintf(stderr,"Proccess with pid=%ld died...\n", (long int)temp->pPid);

		exceptions[0] = 1;
		exceptionsID[0] = temp->pNumber;

		return 0;
	}
	else{
		fprintf(stderr,"No such Proccess");
		return -ENOSYS;
	}
}


/* Create a new task.  */
static void
sched_create_task(char *executable)
{
	processT temp;
	int id = findNewID(procList);
	pid_t p;

	temp = initProc(executable,id);
	addToList(procList,temp);

	p = fork();

	if(p < 0) {
		/* fork failed */
		perror("");
		exit(-1);
	}
	if(p == 0) {
		fprintf(stderr,"A new proccess is created with pid=%ld",(long int)temp->pPid);
		raise(SIGSTOP);
		executeProg(temp);
	}
	if(p>0){
		temp->pPid = p;
		exceptions[1] = 1;
		exceptionsID[1] = temp->pNumber;

	}
}

static int
sched_increase_priority(int id)
{
	processT temp = procList->head;
	int flag = 0;
	temp=findProcID(procList, id, &flag);
	if(flag) {temp->priority = HIGH; fprintf(stderr,"The Process with id=%d and pid=%ld is HIGH!\n", temp->pNumber, (long int)temp->pPid);}
	else fprintf(stderr,"There is no such process!\n");
	return id;
}

static int
sched_decrease_priority(int id)
{
	processT temp = procList->head;
	int flag = 0;
	temp=findProcID(procList, id, &flag);
	if(flag) {temp->priority = LOW; fprintf(stderr,"The Process with id=%d and pid=%ld is LOW!\n", temp->pNumber, (long int)temp->pPid);}
	else fprintf(stderr,"There is no such process!\n");	
	return id;
}

/* Process requests by the shell.  */
static int
process_request(struct request_struct *rq)
{
	switch (rq->request_no) {
		case REQ_PRINT_TASKS:
			sched_print_tasks();
			return 0;

		case REQ_KILL_TASK:
			return sched_kill_task_by_id(rq->task_arg);

		case REQ_EXEC_TASK:
			sched_create_task(rq->exec_task_arg);
			return 0;
			
		case REQ_HIGH_TASK:
			return sched_increase_priority(rq->task_arg);
		
		case REQ_LOW_TASK:
			return sched_decrease_priority(rq->task_arg);

		default:
			return -ENOSYS;
	}
}

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
	
	fprintf(stderr,"Alarm! -.- \n");
	kill(curr->pPid, SIGSTOP);
}

/* 
 * SIGCHLD handler
 */
static void
sigchld_handler(int signum)
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

		if(procList->cnt > 1) explain_wait_status(p, status);

		if (WIFEXITED(status) || WIFSIGNALED(status)) {
			/* A child has died */

			/* The child died because we terminated it with SIGKILL */
			if (exceptions[0] == 1) {
				processT temp;
				exceptions[0] = 0;
				int flag=0;
				temp = findProcID(procList, exceptionsID[0],&flag);
				if(flag) {removeFromList(procList, temp);}
				else fprintf(stderr, "No such proccess!");


				if(procList->head == NULL) {
					printf("All children finished. Exiting...\n");
					exit(EXIT_SUCCESS);
				}
			}

			/* other reasons */
			else {
				processT temp = curr;
				curr = curr->next;
				fprintf(stderr,"Proccess with pid=%ld terminated...\n", (long int)temp->pPid);
				removeFromList(procList, temp);
				

				if(procList->head == NULL) {
					printf("All children finished. Exiting...\n");
					exit(EXIT_SUCCESS);
				}
				if(curr == NULL)
						curr = procList->head;
					
				temp = curr;	
				if(procList->head != NULL){
					while(temp->priority != HIGH){
						if(temp->next == curr || (temp->next == NULL && curr == procList->head)) { temp = curr; break;}
						if(temp->next == NULL) {temp = procList->head; continue;}
						temp = temp->next;
					}
				}
				
				curr = temp;

				/* start alarm */
				if (alarm(SCHED_TQ_SEC) < 0) {
					perror("alarm");
					exit(1);
				}

				kill(curr->pPid, SIGCONT);
			}
		}
		if (WIFSTOPPED(status)) {
				/* A child has stopped due to SIGSTOP/SIGTSTP, etc... */
				curr = curr->next;
				if (curr == NULL)
					curr = procList->head;
					
				processT temp = curr;	
				if(procList->head != NULL){
					while(temp->priority != HIGH){
						if(temp->next == curr || (temp->next == NULL && curr == procList->head)) { temp = curr; break;}
						if(temp->next == NULL) {temp = procList->head; continue;}
						temp = temp->next;
					}
				}
				
				curr = temp;

					/* start alarm */
				//	printf("%d\n", current->pNumber);
					if (alarm(SCHED_TQ_SEC) < 0) {
						perror("alarm");
						exit(1);
				}
			kill(curr->pPid, SIGCONT);
		}
	}
}

/* Disable delivery of SIGALRM and SIGCHLD. */
static void
signals_disable(void)
{
	sigset_t sigset;

	sigemptyset(&sigset);
	sigaddset(&sigset, SIGALRM);
	sigaddset(&sigset, SIGCHLD);
	if (sigprocmask(SIG_BLOCK, &sigset, NULL) < 0) {
		perror("signals_disable: sigprocmask");
		exit(1);
	}
}

/* Enable delivery of SIGALRM and SIGCHLD.  */
static void
signals_enable(void)
{
	sigset_t sigset;

	sigemptyset(&sigset);
	sigaddset(&sigset, SIGALRM);
	sigaddset(&sigset, SIGCHLD);
	if (sigprocmask(SIG_UNBLOCK, &sigset, NULL) < 0) {
		perror("signals_enable: sigprocmask");
		exit(1);
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

static void
do_shell(char *executable, int wfd, int rfd)
{
	char arg1[10], arg2[10];
	char *newargv[] = { executable, NULL, NULL, NULL };
	char *newenviron[] = { NULL };

	sprintf(arg1, "%05d", wfd);
	sprintf(arg2, "%05d", rfd);
	newargv[1] = arg1;
	newargv[2] = arg2;

	raise(SIGSTOP);
	execve(executable, newargv, newenviron);

	/* execve() only returns on error */
	perror("scheduler: child: execve");
	exit(1);
}

/* Create a new shell task.
 *
 * The shell gets special treatment:
 * two pipes are created for communication and passed
 * as command-line arguments to the executable.
 */
static pid_t
sched_create_shell(char *executable, int *request_fd, int *return_fd)
{
	pid_t p;
	int pfds_rq[2], pfds_ret[2];

	if (pipe(pfds_rq) < 0 || pipe(pfds_ret) < 0) {
		perror("pipe");
		exit(1);
	}

	p = fork();
	if (p < 0) {
		perror("scheduler: fork");
		exit(1);
	}

	if (p == 0) {
		/* Child */
		close(pfds_rq[0]);
		close(pfds_ret[1]);
		do_shell(executable, pfds_rq[1], pfds_ret[0]);
		assert(0);
	}
	/* Parent */
	close(pfds_rq[1]);
	close(pfds_ret[0]);
	*request_fd = pfds_rq[0];
	*return_fd = pfds_ret[1];
	
	return p;
}

static void
shell_request_loop(int request_fd, int return_fd)
{
	int ret;
	struct request_struct rq;

	/*
	 * Keep receiving requests from the shell.
	 */
	for (;;) {
		if (read(request_fd, &rq, sizeof(rq)) != sizeof(rq)) {
			perror("scheduler: read from shell");
			fprintf(stderr, "Scheduler: giving up on shell request processing.\n");
			break;
		}

		signals_disable();
		ret = process_request(&rq);
		signals_enable();

		if (write(return_fd, &ret, sizeof(ret)) != sizeof(ret)) {
			perror("scheduler: write to shell");
			fprintf(stderr, "Scheduler: giving up on shell request processing.\n");
			break;
		}
	}
}

int main(int argc, char *argv[])
{
	int nproc;
	pid_t pid;
	int i;
	
	procList = initList();
	
	/* Two file descriptors for communication with the shell */
	static int request_fd, return_fd;

	/* Create the shell. */
	pid = sched_create_shell(SHELL_EXECUTABLE_NAME, &request_fd, &return_fd);
	
	/* TODO: add the shell to the scheduler's tasks */
	curr = initProc(SHELL_EXECUTABLE_NAME, 0);
	curr->pPid = pid;
	addToList(procList, curr);
	
	for(i=1; i<argc; i++){
		curr = initProc(argv[i], i);
		addToList(procList,curr);
	}
	
	/*
	 * For each of argv[1] to argv[argc - 1],
	 * create a new child process, add it to the process list.
	 */

	nproc = argc-1; /* number of proccesses goes here */

	curr = procList->head->next;
	for(i=0; i<nproc; i++){
		pid = fork();
		if(pid<0){
			perror("");
			exit(-1);
		}
		if(pid==0){
			fprintf(stderr,"A new proccess is created with pid=%ld \n",(long int)getpid());
			raise(SIGSTOP);
			executeProg(curr);
		}
		if(pid>0){
			curr->pPid = pid;
			curr = curr->next;
		}
	}

	/* Wait for all children to raise SIGSTOP before exec()ing. */
	wait_for_ready_children(nproc);

	/* Install SIGALRM and SIGCHLD handlers. */
	install_signal_handlers();

	curr = procList->head;
	if (alarm(SCHED_TQ_SEC) < 0) {
		perror("alarm");
		exit(1);
	}
	
	kill(curr->pPid,SIGCONT);

	shell_request_loop(request_fd, return_fd);

	/* Now that the shell is gone, just loop forever
	 * until we exit from inside a signal handler.
	 */
	while (pause())
		;

	/* Unreachable */
	fprintf(stderr, "Internal error: Reached unreachable point\n");
	return 1;
}
