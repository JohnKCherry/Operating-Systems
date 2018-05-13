/***************************************************************/
/*                                                   Operating Systems                                          */
/*                                                   Third Lab Exercise                                          */
/*                                                                                                                           */
/* Exercise 1.2 Mandel                                                                                        */
/*                                                                                                                          */
/* Team: oslaba27                                                                                                */
/*                                                                                                                           */
/* Full Name: Kerasiotis Ioannis, Student ID: 03114951                                   */
/* Full Name: Raftopoulos Evangelos, Student ID: 03114743                           */
/*                                                                                                                          */
/***************************************************************/

/*
 * mandel.c
 *
 * A program to draw the Mandelbrot Set on a 256-color xterm.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>

#include "mandel-lib.h"

#define MANDEL_MAX_ITERATION 100000

/***************************
 * Compile-time parameters *
 ***************************/
 
/*
 * Define a type of a struct which contain:
 * 
 * a. A thread type ID (tid) for each thread which is created
 * b. An integer that resembles current line
 * c. A semaphore for current and next line
 */ 
typedef struct{
	pthread_t tid;
	int line;
	sem_t mutex;
}newThread_t;

/*
 * A pointer to newThread_t variable 
 * for creation of new threads.
 * It is going to be allocated at main function
 * as NTHREAD array.
 */  
newThread_t *thread;

/*
 * Output at the terminal is is x_chars wide by y_chars long
*/
int y_chars = 50;
int x_chars = 90;

/*
 * The part of the complex plane to be drawn:
 * upper left corner is (xmin, ymax), lower right corner is (xmax, ymin)
*/
double xmin = -1.8, xmax = 1.0;
double ymin = -1.0, ymax = 1.0;
	
/*
 * Every character in the final output is
 * xstep x ystep units wide on the complex plane.
 */
double xstep;
double ystep;

/*
 * The number of Threads created for parallel computation
 */
int NTHREADS; 

/*
 * This function computes a line of output
 * as an array of x_char color values.
 */
void compute_mandel_line(int line, int color_val[])
{
	/*
	 * x and y traverse the complex plane.
	 */
	double x, y;

	int n;
	int val;

	/* Find out the y value corresponding to this line */
	y = ymax - ystep * line;

	/* and iterate for all points on this line */
	for (x = xmin, n = 0; n < x_chars; x+= xstep, n++) {

		/* Compute the point's color value */
		val = mandel_iterations_at_point(x, y, MANDEL_MAX_ITERATION);
		if (val > 255)
			val = 255;

		/* And store it in the color_val[] array */
		val = xterm_color(val);
		color_val[n] = val;
	}
}

/*
 * This function outputs an array of x_char color values
 * to a 256-color xterm.
 */
void output_mandel_line(int fd, int color_val[])
{
	int i;
	
	char point ='@';
	char newline='\n';

	for (i = 0; i < x_chars; i++) {
		/* Set the current color, then output the point */
		set_xterm_color(fd, color_val[i]);
		if (write(fd, &point, 1) != 1) {
			perror("compute_and_output_mandel_line: write point");
			exit(1);
		}
	}

	/* Now that the line is done, output a newline character */
	if (write(fd, &newline, 1) != 1) {
		perror("compute_and_output_mandel_line: write newline");
		exit(1);
	}
}

void * compute_and_output_mandel_line(void *arg)
{
	
	int line = *(int *)arg;
	int index;
		
	/*
	 * A temporary array, used to hold color values for the line being drawn
	 */
	 
	int color_val[x_chars];

	for(index=line; index<y_chars; index+=NTHREADS){
	
	/*
	 * Compute mandel line
	 */
		
		compute_mandel_line(index, color_val);
		
	/*
	 * Wait the current line to be printed
	 * and send sign to next semaphore
	 */
	 
		sem_wait(&thread[(index % NTHREADS)].mutex);
		output_mandel_line(STDOUT_FILENO, color_val); // file descriptor for stdout = 1 else I could use STDOUT_FILENO proccessor symbol
		sem_post(&thread[((index % NTHREADS) + 1) % NTHREADS].mutex);
	}
		
	return NULL;
}

/*
 * This function provides user the safety for reset all character
 * attributes before leaving,to ensure the prompt is 
 * not drawn in a fancy color.
 */

void unexpectedSignal(int sign){
	
	signal(sign, SIG_IGN);
	reset_xterm_color(1);
	exit(1);
	
}

int main(void)
{
	int line;

	/*
	 * For question 4, we should modify the program 
	 * in order to terminate normally, if user send 
	 * a signal interrupt from keyboard ( Ctrl + C )
	 */
	
	signal(SIGINT, unexpectedSignal);

	xstep = (xmax - xmin) / x_chars;
	ystep = (ymax - ymin) / y_chars;

	/*
	 * draw the Mandelbrot Set, one line at a time.
	 * Output is sent to file descriptor '1', i.e., standard output.
	 */

	
	/*
	 * Input of Number of threads
	 */
	 
	printf("Enter Number of Threads: ");
	scanf("%d", &NTHREADS);

	if(NTHREADS < 1 || NTHREADS > y_chars){
		printf("Input is not valid\n");
		return 0;
	}
	
	/*
	 * Allocation of Threads and malloc check
	 */
	 
	 thread = (newThread_t *)malloc(NTHREADS * sizeof(newThread_t));
	 if(thread == NULL){
		 perror("");
		 exit(1);
	 }
	
	/*
	 * Initalization of semaphores.
	 * The initalization of first mutex semaphore is set as not asleep,
	 * contrastingly the others semaphores that they must set tp wait.
	 */
	 
	if((sem_init(&thread[0].mutex, 0, 1)) == -1){ // The sem_init function returns 0 on success and -1 on error
		perror("");
		exit(1);
	}
	
	for(line=1; line<NTHREADS; line++){
		if((sem_init(&thread[line].mutex, 0, 0)) == -1){ // The sem_init function returns 0 on success and -1 on error
			perror("");
			exit(1);
		}
	}
	
	/*
	 * Creation of NTHREADS threads passing current line as argument to our 
	 * starting function, compute_and_output_mandel_line.
	 */
	 
	 for(line=0; line<NTHREADS; line++){
		
		thread[line].line = line;
		if((pthread_create(&thread[line].tid, NULL, compute_and_output_mandel_line, &thread[line].line)) != 0){ //The pthreat_create function return 0 on success
			perror("");
			exit(1);
		}
	 }
	
	/*
	 * The following loop makes sure that the terminating of threads will
	 * not be after the done. 
	 */
	
	for(line=0; line<NTHREADS; line++){
		if((pthread_join(thread[line].tid, NULL)) != 0){ // As pthread_create. pthread_join function return 0 on success
			 perror("");
			 exit(1);
		}
	}	
	
	/*
	 * Destroy the semaphores
	 */
	
	for (line = 0; line < NTHREADS; line++) {
		sem_destroy(&thread[line].mutex);
	}

	free(thread);
	
	reset_xterm_color(1);
	return 0;
}
