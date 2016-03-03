/**************************************************************************
    Travail pratique No 2 : Thread utilisateurs

    Ce fichier est votre implémentation de la librarie des threads utilisateurs.

	Systemes d'explotation GLO-2001
	Universite Laval, Quebec, Qc, Canada.
	(c) 2016 Philippe Giguere
 **************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ucontext.h>
#include <time.h>
#include <signal.h>
#include <sys/time.h>
#include "ThreadUtilisateur.h"

/* Définitions privées, donc pas dans le .h, car l'utilisateur n'a pas besoin de
   savoir ces détails d'implémentation. OBLIGATOIRE. */
typedef enum {
	THREAD_EXECUTE=0,
	THREAD_PRET,
	THREAD_BLOQUE,
	THREAD_TERMINE
} EtatThread;

#define TAILLE_PILE 8192   // Taille de la pile utilisée pour les threads

/* Structure de données pour créer une liste chaînée simple sur les threads qui ont fait un join.
   Facultatif */
typedef struct WaitList {
	struct TCB *pThreadWaiting;
	struct WaitList *pNext;
} WaitList;

/* TCB : Thread Control Block. Cette structure de données est utilisée pour stocker l'information
   pour un thread. Elle permet aussi d'implémenter une liste doublement chaînée de TCB, ce qui
   facilite la gestion et permet de faire un ordonnanceur tourniquet sans grand effort.  */
typedef struct TCB {  // Important d'avoir le nom TCB ici, sinon le compilateur se plaint.
	tid                 id;        // Numero du thread
	EtatThread			etat;      // Etat du thread
	ucontext_t          ctx;       // Endroit où stocker le contexte du thread
	time_t              WakeupTime; // Instant quand réveiller le thread, s'il dort, en epoch time.
	struct TCB         *pSuivant;   // Liste doublement chaînée, pour faire un buffer circulaire
	struct TCB         *pPrecedant; // Liste doublement chaînée, pour faire un buffer circulaire
	struct WaitList	   *pWaitListJoinedThreads; // Liste chaînée simple des threads en attente.
} TCB;

// Pour que les variables soient absolument cachées à l'utilisateur, on va les déclarer static
static TCB *gpThreadCourant = NULL;	 // Thread en cours d'execution
static TCB *gpNextToExecuteInCircularBuffer = NULL;
static int gNumberOfThreadInCircularBuffer = 0;
static int gNextThreadIDToAllocate = 0;
static WaitList *gpWaitTimerList = NULL;
static TCB *gThreadTable[MAX_THREADS]; // Utilisé par la fonction ThreadID()
static char gEtatTable[4] = {'E', 'P', 'B', 'T'};

/* Cette fonction ne fait rien d'autre que de spinner un tour et céder sa place. C'est l'équivalent
   pour un système de se tourner les pouces. */
void IdleThreadFunction(void *arg) {
	struct timespec SleepTime, TimeRemaining;
	SleepTime.tv_sec = 0;
	SleepTime.tv_nsec = 250000000;
	while (1) {
		printf("                #########  Idle Thread 0 s'exécute et va prendre une pose de 250 ms... #######\n");
		/* On va dormir un peu, pour ne pas surcharger inutilement le processus/l'affichage. Dans un
		   vrai système d'exploitation, s'il n'y a pas d'autres threads d'actifs, ce thread demanderait au
		   CPU de faire une pause, car il n'y a rien à faire. */
	    nanosleep(&SleepTime,&TimeRemaining); // nanosleep interfere moins avec les alarmes.
		ThreadCeder();
	}
}

void retirerDuBufferCirculaire(TCB *pThread) {
	// Mettre le suivant en tête
	pThread->pPrecedant->pSuivant = pThread->pSuivant;
	pThread->pSuivant->pPrecedant = pThread->pPrecedant;

	// Enlever du cercle le thread courant
	pThread->pSuivant = NULL;
	pThread->pPrecedant = NULL;

	gNumberOfThreadInCircularBuffer--;
}

void ajouterAuBufferCirculaire(TCB *pThread) {
	if (pThread->id == (tid)0) {
		pThread->pPrecedant = NULL;
		pThread->pSuivant = NULL;
	} else if (pThread->id == (tid)1) {
		pThread->pSuivant = gThreadTable[0];
		pThread->pSuivant->pPrecedant = pThread;
		pThread->pPrecedant = gThreadTable[0];
		pThread->pPrecedant->pSuivant = pThread;
	}
	else {
		pThread->pSuivant = gpThreadCourant->pSuivant;
		pThread->pSuivant->pPrecedant = pThread;
		pThread->pPrecedant = gpThreadCourant;
		pThread->pPrecedant->pSuivant = pThread;
	}
	gNumberOfThreadInCircularBuffer++;
	pThread->etat = THREAD_PRET;
}

/* ******************************************************************************************
                                   T h r e a d I n i t
   ******************************************************************************************/
int ThreadInit(void){
	printf("\n  ******************************** ThreadInit()  ******************************** \n");
	// THREAD IDLE
  ThreadCreer(*IdleThreadFunction, 0);

	// THREAD MAIN
  TCB *tcb_main = (TCB *) malloc(sizeof(TCB));
  tcb_main->id = gNextThreadIDToAllocate;
	gNextThreadIDToAllocate++;
	gThreadTable[tcb_main->id] = tcb_main;
	ajouterAuBufferCirculaire(tcb_main);
	gpThreadCourant = tcb_main;
	return 1;
}


/* ******************************************************************************************
                                   T h r e a d C r e e r
   ******************************************************************************************/
tid ThreadCreer(void (*pFuncThread)(void *), void *arg) {
	printf("\n  ******************************** ThreadCreer(%p,%p) ******************************** \n",pFuncThread,arg);
	// CREATION DU THREAD
	TCB *tcb_nouveau = (TCB *) malloc(sizeof(TCB));
	tcb_nouveau->id = gNextThreadIDToAllocate;
	gNextThreadIDToAllocate++;
	gThreadTable[tcb_nouveau->id] = tcb_nouveau;
	tcb_nouveau->pWaitListJoinedThreads = NULL;
	getcontext(&tcb_nouveau->ctx);

	char *pile = malloc(TAILLE_PILE);
	if (pile == NULL) {
		return -1;
	}
	tcb_nouveau->ctx.uc_stack.ss_sp = pile;
	tcb_nouveau->ctx.uc_stack.ss_size = TAILLE_PILE;
	makecontext(&tcb_nouveau->ctx, (void *) pFuncThread, 1, arg);

	ajouterAuBufferCirculaire(tcb_nouveau);

	gpNextToExecuteInCircularBuffer = tcb_nouveau;
	return tcb_nouveau->id;
}

/* ******************************************************************************************
                                   T h r e a d C e d e r
   ******************************************************************************************/
void ThreadCeder(void) {
	printf("\n  ******************************** ThreadCeder()  ******************************** \n");
	printf("----- Etat de l'ordonnanceur avec %d threads -----\n", gNumberOfThreadInCircularBuffer);
	struct TCB *i_tcb = gpNextToExecuteInCircularBuffer;
	struct WaitList *i_waitList = gpWaitTimerList;
	do {
		char debut[20] = "\t";
		char special[20] = "";
		struct WaitList *i_threadWaitList = i_tcb->pWaitListJoinedThreads;
		if (i_tcb == gpNextToExecuteInCircularBuffer) strncpy(debut, "prochain->", sizeof(debut));
		if (i_tcb->id == (tid)0) strncpy(special, "*Special Idle Thread*", sizeof(special));
		printf("| %s\tThreadID:%d\t État:%c %s\t WaitList", debut, i_tcb->id, gEtatTable[i_tcb->etat], special);
		while(i_threadWaitList) {
			printf("-->(%d)", i_threadWaitList->pThreadWaiting->id);
			i_threadWaitList = i_threadWaitList->pNext;
		}
		printf("\n");
		i_tcb = i_tcb->pSuivant;
	} while(i_tcb != gpNextToExecuteInCircularBuffer);
	printf("----- Liste des threads qui dorment, epoch time=%d -----\n", (int)time(NULL));
	while (i_waitList) {
		printf("| \t\tThreadID:%d\t État:%c\t WakeTime=%d\t WaitList", i_waitList->pThreadWaiting->id, gEtatTable[i_waitList->pThreadWaiting->etat], (int)i_waitList->pThreadWaiting->WakeupTime);
		struct WaitList *i_threadWaitList = i_waitList->pThreadWaiting->pWaitListJoinedThreads;
		while(i_threadWaitList) {
			printf("-->(%d)", i_threadWaitList->pThreadWaiting->id);
			i_threadWaitList = i_threadWaitList->pNext;
		}
		printf("\n");
		i_waitList = i_waitList->pNext;
	}
	printf("-------------------------------------------------\n");

	WaitList *precedant = NULL;
	while(gpWaitTimerList && gpWaitTimerList->pThreadWaiting) {
		if (gpWaitTimerList->pThreadWaiting->WakeupTime <= time(NULL)) {
			// Retirer de cette liste
			if(precedant != NULL) {
				if(gpWaitTimerList->pNext != NULL) {
					precedant->pNext = gpWaitTimerList->pNext;
				} else {
					precedant->pNext = NULL;
				}
			}
			ajouterAuBufferCirculaire(gpWaitTimerList->pThreadWaiting);
		}
		precedant = gpWaitTimerList;
		gpWaitTimerList = gpWaitTimerList->pNext;
	}
	// Faire du garbage collection
	while(gpNextToExecuteInCircularBuffer->etat == THREAD_TERMINE) {
		TCB * pThread = gpNextToExecuteInCircularBuffer;
		retirerDuBufferCirculaire(pThread);
		// Désallouer sa pile
		free(pThread->ctx.uc_stack.ss_sp);
		// Retirer du ThreadTable
		gThreadTable[pThread->id] = NULL;
		// On passe au prochain thread à exécuter
		gpNextToExecuteInCircularBuffer = pThread->pSuivant;
		// Desallouer le TCB
		free(pThread);
	}
	TCB *oldCourrant = gpThreadCourant;
	oldCourrant->etat = THREAD_PRET;
	gpThreadCourant = gpNextToExecuteInCircularBuffer;
	gpThreadCourant->etat = THREAD_EXECUTE;
	gpNextToExecuteInCircularBuffer = gpThreadCourant->pSuivant;
	swapcontext(&oldCourrant->ctx, &gpThreadCourant->ctx);
}


/* ******************************************************************************************
                                   T h r e a d J o i n d r e
   ******************************************************************************************/
int ThreadJoindre(tid ThreadID){
	printf("\n  ******************************** ThreadJoindre(%d)  ******************************* \n",ThreadID);
	// Obtenir le thread par son tid
	TCB *threadAJoindre = gThreadTable[ThreadID];
	if (threadAJoindre == NULL || threadAJoindre->etat == THREAD_TERMINE) {
		return -1;
	}
	// Met le thread comme bloquer
	gpThreadCourant->etat = THREAD_BLOQUE;
	// Obtient l'espace nécessaire pour waitList
	WaitList *waitList = (struct WaitList *) malloc(sizeof(struct WaitList));
	waitList->pThreadWaiting = gpThreadCourant;
	waitList->pNext = threadAJoindre->pWaitListJoinedThreads;
	threadAJoindre->pWaitListJoinedThreads = waitList;

	// On continue dans l'ordonnanceur
	ThreadCeder();

	// On retourne 1, car tout a fonctionné
	return 1;
}


/* ******************************************************************************************
                                   T h r e a d Q u i t t e r
   ******************************************************************************************/
void ThreadQuitter(void){
	printf("  ******************************** ThreadQuitter(%d)  ******************************** \n",gpThreadCourant->id);
	// Mettre le thread comme étant terminer
	gpThreadCourant->etat = THREAD_TERMINE;

	// Réveille les threads en attente
	WaitList *waitList = gpThreadCourant->pWaitListJoinedThreads;
	while (waitList != NULL) {
		waitList->pThreadWaiting->etat = THREAD_PRET;
		waitList = waitList->pNext;
	}

	retirerDuBufferCirculaire(gpThreadCourant);

	ThreadCeder();
	printf(" ThreadQuitter:Je ne devrais jamais m'exectuer! Si je m'exécute, vous avez un bug!\n");
	return;
}

/* ******************************************************************************************
                                   T h r e a d I d
   ******************************************************************************************/
tid ThreadId(void) {
	// Libre à vous de la modifier. Mais c'est ce que j'ai fait dans mon code, en toute simplicité.
	return gpThreadCourant->id;
}

/* ******************************************************************************************
                                   T h r e a d D o r m i r
   ******************************************************************************************/
void ThreadDormir(int secondes) {
	printf("\n  ******************************** ThreadDormir(%d)  ******************************** \n",secondes);
	// Mettre le thread comme bloquer
	gpThreadCourant->etat = THREAD_BLOQUE;
	// Ajouter le nombre de seconde à dormir
	gpThreadCourant->WakeupTime = time(NULL) + secondes;
	// Retirer du buffer circulaire
	retirerDuBufferCirculaire(gpThreadCourant);
	// Allouer l'espace pour la nouvelle waitList.
	WaitList *waitList = (struct WaitList *)malloc(sizeof(struct WaitList));
	// Mettre le thread courant à dormir
	waitList->pThreadWaiting = gpThreadCourant;
	// Mettre le suivant comme étant le premier de l'autre liste.
	waitList->pNext = gpWaitTimerList;
	// Ajouter ensuite l'élément dans la liste du  WaitTimerList.
	gpWaitTimerList = waitList;
	// Céder la place à un autre thread.
	ThreadCeder();
	return;
}
