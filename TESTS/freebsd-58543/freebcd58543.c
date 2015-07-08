#include <stdio.h>      /* Input/Output */
#include <stdlib.h>     /* General Utilities */
#include <pthread.h>    /* POSIX Threads */
#include <unistd.h>

typedef struct file_pointer{
  int f_ops;
  int *f_vnode;
}fptr;

//Use f_vnode in this function
void fo_kqfilter(void *ptr){
  fptr *fp = (fptr *)ptr;
  if (fp->f_ops == 0){
    return;
  }
  int *get = fp->f_vnode; //Read fp->f_vnode and use it
  printf("read f_vnode: %d\n", *get);

  return;
}

//Assign non-zero value to f_ops and allocate a vnode for this opening file
void devfs_open(void *ptr){
  fptr *fp = (fptr *)ptr;
  int val = 100;
  int *vp = &val;
  
  fp->f_ops = 1;
  sleep(1);
  fp->f_vnode = vp;

  return;
}

//Set f_vnode to NULL initially
void falloc(void *ptr){
  fptr *fp = (fptr *)ptr;
  fp->f_ops = 0;
  fp->f_vnode = NULL;

  return;
}

int main(){
  pthread_t thread1, thread2;
  fptr *fp = (fptr*) malloc(sizeof(fptr));

  falloc(fp);
 // fp->f_ops = 0;
 // fp->f_vnode = NULL;

  printf("%d\n", fp->f_ops);

  pthread_create (&thread1, NULL, (void *) &fo_kqfilter, (void *) fp);
  pthread_create (&thread2, NULL, (void *) &devfs_open, (void *) fp);

  pthread_join(thread1, NULL);
  pthread_join(thread2, NULL);

  return 0;

}
