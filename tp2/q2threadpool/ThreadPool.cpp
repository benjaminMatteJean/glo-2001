#include "ThreadPool.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* Thunk : In computer programming, a thunk is a subroutine that is created, often automatically,
   to assist a call to another subroutine. Thunks are primarily used to represent an additional
   calculation that a subroutine needs to execute, or to call a routine that does not support the
   usual calling mechanism. http://en.wikipedia.org/wiki/Thunk */
typedef struct {
	ThreadPool *pThreadPool; // pointeur sur l'objet ThreadPool
	int ThreadNum; // Numéro du thread, de 0 à n
} threadArg;

void *Thunk(void *arg) {
	threadArg *pThreadArg = (threadArg *)arg;
	ThreadPool *pThreadPool;
	pThreadPool = static_cast<ThreadPool*>(pThreadArg->pThreadPool);
	pThreadPool->MyThreadRoutine(pThreadArg->ThreadNum);
}


/* void ThreadPool(unsigned int nThread)
 Ce constructeur doit initialiser le thread pool. En particulier, il doit initialiser les variables
 de conditions et mutex, et démarrer tous les threads dans ce pool, au nombre spécifié par nThread.
 IMPORTANT! Vous devez initialiser les variables de conditions et le mutex AVANT de créer les threads
 qui les utilisent. Sinon vous aurez des bugs difficiles à comprendre comme des threads qui ne débloque
 jamais de phtread_cond_wait(). */
ThreadPool::ThreadPool(unsigned int nThread) {
	// Cette fonction n'est pas complète! Il vous faut la terminer!
	// Initialisation des membres
	PoolDoitTerminer = false;
	nThreadActive = nThread;
	bufferValide = true;
	buffer = 0;

	// Initialisation du mutex et des variables de conditions.
	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&CondThreadRienAFaire,0);
	pthread_cond_init(&CondProducteur,0);

	// Création des threads. Je vous le donne gratuit, car c'est un peu plus compliqué que vu en classe.
	pTableauThread        = new pthread_t[nThread];
	threadArg *pThreadArg = new threadArg[nThread];
	int i;
	for (i=0; i < nThread; i++) {
		pThreadArg[i].ThreadNum = i;
		pThreadArg[i].pThreadPool = this;
    	printf("ThreadPool(): en train de creer thread %d\n",i);
   		int status = pthread_create(&pTableauThread[i], NULL, Thunk, (void *)&pThreadArg[i]);
   		if (status != 0) {
   			printf("oops, pthread a retourne le code d'erreur %d\n",status);
   			exit(-1);
    	}
    }
}

/* Destructeur ThreadPool::~ThreadPool()
   Ce destructeur doit détruire les mutex et variables de conditions. */
ThreadPool::~ThreadPool() {
	// À compléter
	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy(&CondProducteur);
	pthread_cond_destroy(&CondThreadRienAFaire);
	delete [] pTableauThread;
}

/* void ThreadPool::MyThreadRoutine(int myID)
   Cette méthode est celle qui tourne pour chacun des threads crées dans le constructeur, et qui est
   appelée par la fontion thunk. Cette méthode est donc effectivement le code du thread consommateur,
   qui ne doit quitter qu’après un appel à la méthode Quitter(). Si le buffer est vide, MyThreadRoutine
   doit s'arrêter (en utilisant une variable de condition). Le travail à accomplir est un sleep() d'une
   durée spécifiée dans le buffer.
   */
void ThreadPool::MyThreadRoutine(int myID) {
	// À compléter
	printf("Thread %d commence!\n", myID);

	while (!PoolDoitTerminer) {
		pthread_mutex_lock(&mutex);

		if (!bufferValide) {
			pthread_cond_wait(&CondThreadRienAFaire,&mutex);
		}

		if(!PoolDoitTerminer) {
			printf("Thread %d récupère l'item %d!\n",myID,buffer);

			int currentBuffer = buffer;
			bufferValide = false;

			pthread_cond_signal(&CondProducteur);
			pthread_cond_signal(&CondThreadRienAFaire);

			pthread_mutex_unlock(&mutex);

			printf("Thread %d va dormir %d sec.\n",myID,currentBuffer);

			sleep(currentBuffer);
		}
	}

	printf("############ Thread %d termine!################\n",myID);

}

/* void ThreadPool::Inserer(unsigned int newItem)
   Cette méthode est appelée par le thread producteur pour mettre une tâche à exécuter dans le buffer
   (soit le temps à dormir pour un thread). Si le buffer est marqué comme plein, il faudra dormir
   sur une variable de condition. */
void ThreadPool::Inserer(unsigned int newItem) {
	// À compléter
	pthread_mutex_lock(&mutex);

	if (bufferValide) {
		pthread_cond_wait(&CondProducteur,&mutex);
	}

	bufferValide = true;
	buffer = newItem;
	pthread_cond_signal(&CondThreadRienAFaire);
	pthread_mutex_unlock(&mutex);
}

/* void ThreadPool::Quitter()
   Cette fonction est appelée uniquement par le producteur, pour indiquer au thread pool qu’il n’y
   aura plus de nouveaux items qui seront produits. Il faudra alors que tous les threads terminent
   de manière gracieuse. Cette fonction doit bloquer jusqu’à ce que tous ces threads MyThreadRoutine
   terminent, incluant ceux qui étaient bloqués sur une variable de condition. */
void ThreadPool::Quitter() {
	// À compléter
	pthread_mutex_lock(&mutex);

	while (bufferValide) {
		pthread_cond_wait(&CondThreadRienAFaire,&mutex);
	}

	PoolDoitTerminer = true;
	pthread_cond_broadcast(&CondThreadRienAFaire);
	pthread_mutex_unlock(&mutex);


	for (int i = 0; i < nThreadActive; i++) {
		 pthread_join(pTableauThread[i],NULL);
	}
}
