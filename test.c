#include    "unp.h"
#include	<time.h>

int     counter;                /* incremented by threads */
 pthread_mutex_t counter_mutex = PTHREAD_MUTEX_INITIALIZER;
 
void *doit(void *);

int main(int argc, int **argv)
{
   pthread_t pid;
   
   Pthread_create(&pid, NULL, &doit, NULL);
   int i, val;
   for (i=0; i<10; i++)
   { 
        sleep(3);
   	printf("%s\n", " I am the main thread ");
        Pthread_mutex_lock(&counter_mutex);
   	val = counter;
   	printf("%d\n", val + 5);
   	counter = val + 5;
   	Pthread_mutex_unlock(&counter_mutex);
   }
   
  exit(0);
}

void *doit(void *vptr)
{
 int i, count;
 for (i=0; i<10; i++)
   { 
 	sleep(1);
	fprintf(stdout, "%s\n", " I am the child thread ");
        Pthread_mutex_lock(&counter_mutex);
   	count = counter;
   	printf("%d\n", count - 1);
   	counter = count - 1;
	Pthread_mutex_unlock(&counter_mutex);   
  }
  exit(0);
}
