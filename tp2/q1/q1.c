#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <sys/syscall.h>
#include <sys/resource.h>

typedef struct {
  int priority;
} Params;

void *changePriority(void * args) {
   Params *params  = (Params *) args;
   int priority = params->priority;
   int threadID = syscall(SYS_gettid);
   int ret = setpriority(PRIO_PROCESS, threadID, priority);
   printf("Thread %d avec la priority %d \n",threadID,priority);
   printf("setpriority():%d\n", ret);
   printf("errno: %d\n",errno);
   while (1);
}


int main(int argc, char const *argv[]) {
  const int NB_THREADS = 5;
  pthread_t threads[NB_THREADS];
  Params params[NB_THREADS];
  int i;
  
  for (i = 0; i < NB_THREADS; i++) {
    printf("main(): Création du thread %d\n",i);
    int valeur = atoi(argv[i+1]);
    params[i].priority = valeur;

    int status = pthread_create(&threads[i], NULL, changePriority, (void *)&params[i]);

    if(status != 0) {
        printf("Erreur, code : %d\n",status);
        exit(EXIT_FAILURE);
    }
  }

  for (i = 0; i < NB_THREADS; i++) {
    pthread_join(threads[i],NULL);
  }

  return 0;
}
