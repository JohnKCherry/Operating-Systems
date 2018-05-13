#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "proc-common.h"

#define SLEEP_PROC_SEC  10

/*
 * Create this process tree:
 * A-+-B---D
 *   `-C
 */
void fork_procs(void)
{
	/*
	 * initial process is A.
	 */

	change_pname("A");                         // onomazoume thnn nea diergasia "A"
	printf("A with PID = %ld is created...\n", (long)getpid());

	pid_t pid_b, pid_c, pid_d;
	int status;
	
	printf( "A with PID = %ld: Creating child B...\n", (long)getpid());	
	pid_b = fork();                            //dhmiourgoume nea diergasia thn opoia tha onomasoume "B"
	if (pid_b < 0){                            //elegxos gia thn dimiourgia ths diergasias
		perror("B: fork");
		exit(1);
	}
	if (pid_b == 0){                             //edo tha bei to child
		change_pname("B");                  //thn onomazoume "B"
		printf("B with PID = %ld is created...\n", (long)getpid());
		printf( "B with PID = %ld: Creating child D...\n", (long)getpid());
		pid_d = fork();                     //dimiourgia neas diergasias thn opoia tha onomasoume "D"
		if (pid_d < 0){                     //elegxos gia thn dimiourgia ths diergasias
			perror("D: fork");
			exit(1);
		}
		if (pid_d == 0){                    //edo tha bei to child  
			change_pname("D");          //thn onomatizoume "D"
			printf("D with PID = %ld is created...\n", (long)getpid());
			printf( "D with PID = %ld is ready to Sleep...\n",(long)getpid());
			sleep(SLEEP_PROC_SEC);      //bazoume thn "D" pou einai fullo na koimatai oste na dimiourgithoun kai oi alles
			printf("D with PID = %ld is ready to terminate...\nD: Exiting...\n", (long)getpid());
			exit(13);
		}
		
		waitpid(pid_d, &status, 0);           //perimenei o pateras mexri na pethanei to D gia na sunexisei
		explain_wait_status(pid_d, status);   //eksigei ton logo pou pethane to paidi me id pou tou dinoume os 1o orisma
		printf("B with PID = %ld is ready to terminate...\nB: Exiting...\n", (long)getpid());
		exit(19);	
	}
	
	printf( "A with PID = %ld: Creating child C...\n", (long)getpid());
	pid_c = fork();                         //dhmiourgia neas diergasias thn opoia tha onomasoume "C"   
	if (pid_c < 0) {                        //elegxos gia thn dimiourgia ths diergasias
		perror("C: fork");
		exit(1);
	}
	if (pid_c == 0){                        //edo tha bei to paidi dhladh h C
		change_pname("C");              //thn onomazoume "C"
		printf("C with PID = %ld is created...\n", (long)getpid());
		printf("C with PID = %ld is ready to sleep...\n",(long)getpid());
		sleep(SLEEP_PROC_SEC);           //einai fullo ara tha thn baloume na koimithei
		printf("C with PID = %ld is ready to terminate...\nC: Exiting...\n", (long)getpid());
		exit(17);
	}
	
	waitpid(pid_c, &status, 0);           //perimenei o pateras mexri na pethanei to paidi C gia na sunexisei
	explain_wait_status(pid_c, status);   //eksigei ton logo pou pethane to paidi pou tou dinoume os 1o orisma
	
	waitpid(pid_b, &status, 0);           //perimenei o pateras mexri na pethanei to paidi B gia na sunexisei
	explain_wait_status(pid_b, status);   //eksigei ton lolo pou pethane to B
	
	printf("A with PID = %ld is ready to terminate...\nA: Exiting...\n", (long)getpid());
	exit(16);
}

/*
 * The initial process forks the root of the process tree,
 * waits for the process tree to be completely created,
 * then takes a photo of it using show_pstree().
 */
int main(void)
{
	pid_t pid;
	int status;

	/* Fork root of process tree */
	pid = fork();                      //dhmioyrgia neas diergasias
	if (pid < 0) {                     //elegxos gia to an ontos dimiourgithike h nea diergasia
		perror("main: fork");
		exit(1);
	}
	if (pid == 0) {                    //sunthikh pou tha bei otan einai child afou epistrefei pin=0
		/* Child */
		fork_procs();            
		exit(1);
	}

	/*
	 * Father
	 */
	 
	/* Print the process tree root at pid */
	show_pstree(pid);                  //emfanizei to tree diergasion pou exei dhmiourgithei
	
	/* Wait for the root of the process tree to terminate */
	waitpid(pid, &status, 0);          //h arxikh diergasia perimenei mexri na pethanei to paidi tou diladi to A
	explain_wait_status(pid, status);  //eksigei ton logo pou pethane o A

	return 0;
}

