/***************************************************************/
/*                      Operating Systems                      */
/*                     First Lab Exercise                      */
/*                                                             */
/* Exercise 1.1 Zing2 source code for zing function            */
/*                                                             */
/* Team: Oslaba27                                              */ 
/*                                                             */
/* Full Name: Kerasiotis Ioannis, Student ID: 03114951         */
/* Full Name: Raftopouloes Evangelos, Student ID: 03114743     */
/*                                                             */
/***************************************************************/


#include <stdio.h> // include stdio.h header for performing input and output
#include <unistd.h> // include unistd.h header for access to operating system

void zing(){
  char *name; // a string buffer which stores pointer that returns from getlogin() 
  name = getlogin(); // calling getlogin() function
  if (name == NULL) // if function return null pointer
    perror("getlogin() error"); // return error message
  else
    printf("Hi, %s !\n", name); // else print this message with login name
}
