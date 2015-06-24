#include <unistd.h>     /* Symbolic Constants */
#include <sys/types.h>  /* Primitive System Data Types */ 
#include <errno.h>      /* Errors */
#include <stdio.h>      /* Input/Output */
#include <stdlib.h>     /* General Utilities */
#include <pthread.h>    /* POSIX Threads */
#include <string.h>     /* String handling */

int dying = 0;

void _libsafe_die ( void *ptr )
{
  dying = 1;
  
  sleep(1); 
  
  dying = 0;
  pthread_exit(0);
}

void _libsafe_stackVariableP (void *ptr)
{
  char dest[40];
  if(dying == 1) {
    strcpy(dest, "shellcode");
  }
}

int main()
{
  pthread_t thread1, thread2;  /* thread variables */
  
  pthread_create (&thread1, NULL, (void *) &_libsafe_die, (void *) NULL);
  pthread_create (&thread2, NULL, (void *) &_libsafe_stackVariableP, (void *) NULL);

  pthread_join(thread1, NULL);
  pthread_join(thread2, NULL);
            
  exit(0);
}
