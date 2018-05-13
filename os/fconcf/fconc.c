/***************************************************************/
/*                      Operating Systems                      */
/*                     First Lab Exercise                      */
/*                                                             */
/* Exercise 1.2 fconc program                                  */
/*                                                             */
/* Team: Oslaba27                                              */
/*                                                             */
/* Full Name: Kerasiotis Ioannis, Student ID: 03114951         */
/* Full Name: Raftopoulos Evangelos, Student ID: 03114743      */
/*                                                             */
/***************************************************************/


/************************ Libraries ****************************/

#include <sys/types.h> 
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/***************************************************************/

#ifndef BUFF_SIZE // allows "gcc -D" to override definition
#define BUFF_SIZE 1024 // define a constant value at BUFF SIZE
#endif

/************************ Functions ****************************/

void write_file(int fd, const char *infile); //	a function which writes the content of file with the "infile" name to fd file
void doWrite(int fd, const char *buff, ssize_t len); // a function which figurate the writing of file

/***************************************************************/

int main (int argc, char **argv){
	
	if (argc<3 || argc>4){								  // Check if the number of arguments is acceptable 
		printf("Usage: ./fconc infile1 infile2 [outfile (default:fconc.out)]\n"); // if not print this message 
		exit(1);								  // and end the program
	}
	
	int fd, osFlags, filePerms;	

	osFlags = O_CREAT | O_WRONLY | O_TRUNC; // flags for creating a file (if it doesn't exist), writing (not reading)
						// and truncate the existing file to zero 
	filePerms = S_IRUSR | S_IWUSR;		// Read(0400)  and write(0200) permission bit for the owner of the file
	
	if (argv[3] == NULL) argv[3] = "fconc.out"; // in case there isn't a third argument give the by default value "fconc.out"
	fd = open(argv[3], osFlags, filePerms);    // open (or create) a file identified by a path name and return a file descriptor (fd) 
	if (fd < 0){
		perror("Open"); //in case of error display an error message and terminate the program
		exit(1);
	}
	
	write_file(fd, argv[1]); // call function write_file(2) for first argument
	write_file(fd, argv[2]); // call function write_file(2) for second argument
	
	if(close(fd) < 0){ 		//close file descriptοr
		perror("Close");	//in case of wrong
		exit(-1);
	}
	
	return (0);

}

void write_file(int fd, const char *infile){
	
	int fd1; // a file descriptor for the file which is opened and read
	char buff[BUFF_SIZE]; // a current buffer for reading and writing the content
	ssize_t rcnt; //This data type is used to represent the sizes of blocks that can be read or written in a single operation
	
	fd1 = open(infile, O_RDONLY); // opening the file with pathname infile (only for reading) and returning at file descriptor fd1 
	if (fd1 < 0){		     // in case of wrong opening (for example, if there is not existing file)
		perror(infile);	    // display error message
		exit(1);	   // and terminate program
	}
	
	while(((rcnt = read(fd1, buff, BUFF_SIZE)) != 0 )) { // if the file descriptor isn't at the end of file (if fd==0 it is at end)
		if (rcnt == -1){			    // if there is wrong with reading
			perror("Read");			   // display error
			exit(1);			  // and terminate the program
		}
		doWrite(fd,buff,rcnt); //calling the function to write the temporary content of the buffer to file

	}
	
	if (close(fd1) < 0){		 //close file descriptpr
                perror("Close");        //in case of wrong
                exit(-1);	       // and terminate
        }

}

void doWrite(int fd, const char *buff, ssize_t len){
	if (write(fd,buff,len) != len){ 		  // write the content and check if it is right
		perror("Couldn't write whole buffer!");  // else display error
		exit(1);			 	// and terminate
	}
}
