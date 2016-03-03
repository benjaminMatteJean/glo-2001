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


/* ******************************************************************************************
                                   T h r e a d I n i t
   ******************************************************************************************/
int ThreadInit(void){
	printf("\n  ******************************** ThreadInit()  ******************************** \n");
	// Initialisation de la WaitList
	gpWaitTimerList = (WaitList *) malloc(sizeof(WaitList));
	gpWaitTimerList->pNext = NULL;
	gpWaitTimerList->pThreadWaiting = NULL;

	// THREAD IDLE
    ThreadCreer(*IdleThreadFunction, 0);
	// THREAD MAIN
    TCB *tcb_main = (TCB *) malloc(sizeof(TCB));
    tcb_main->id = gNextThreadIDToAllocate;
	gNextThreadIDToAllocate++;
	gThreadTable[tcb_main->id] = tcb_main;

	// Ajout du main dans le buffer circulaire
	tcb_main->pSuivant = gpThreadCourant;
	tcb_main->pSuivant->pPrecedant = tcb_main;
	tcb_main->pPrecedant = gThreadTable[0];
	tcb_main->pPrecedant->pSuivant = tcb_main;
	gNumberOfThreadInCircularBuffer++;

	tcb_main->etat = THREAD_PRET;
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

	// AJOUT DANS LE BUFFER CIRCULAIRE
	if (tcb_nouveau->id == (tid)0) {
		tcb_nouveau->pPrecedant = NULL;
		tcb_nouveau->pSuivant = NULL;
	} else {
		tcb_nouveau->pSuivant = gpThreadCourant;
		tcb_nouveau->pSuivant->pPrecedant = tcb_nouveau;
		tcb_nouveau->pPrecedant = gThreadTable[0];
		tcb_nouveau->pPrecedant->pSuivant = tcb_nouveau;
	}
	gNumberOfThreadInCircularBuffer++;

	tcb_nouveau->etat = THREAD_PRET;
	gpNextToExecuteInCircularBuffer = tcb_nouveau;
	gpThreadCourant = gpNextToExecuteInCircularBuffer;
	return gpThreadCourant->id;
}

/* ******************************************************************************************
                                   T h r e a d C e d e r
   ******************************************************************************************/
void ThreadCeder(void){
	printf("\n  ******************************** ThreadCeder()  ******************************** \n");

	WaitList *precedant = NULL;

	while(gpWaitTimerList->pThreadWaiting) {
		if (gpWaitTimerList->pThreadWaiting->WakeupTime <= time(NULL)) {
			// Retirer de cette liste
			if(precedant != NULL) {
				if(gpWaitTimerList->pNext != NULL) {
					precedant->pNext = gpWaitTimerList->pNext;
				} else {
					precedant->pNext = NULL;
				}
			}
			// Changer son état
			gpWaitTimerList->pThreadWaiting->etat = THREAD_PRET;
			// Placer dans le buffer circulaire
			gpWaitTimerList->pThreadWaiting->pSuivant = gpThreadCourant->pSuivant;
			gpWaitTimerList->pThreadWaiting->pPrecedant = gpThreadCourant;
			gpThreadCourant->pSuivant->pPrecedant = gpWaitTimerList->pThreadWaiting;
			gpThreadCourant->pSuivant = gpWaitTimerList->pThreadWaiting;
			gNumberOfThreadInCircularBuffer++;
		}
		precedant = gpWaitTimerList;
		gpWaitTimerList = gpWaitTimerList->pNext;
	}
	// Faire du garbage collection
	while(gpNextToExecuteInCircularBuffer->etat == THREAD_TERMINE) {
		TCB * pThread = gpNextToExecuteInCircularBuffer;
		// On l'enlève du buffer circulaire
		pThread->pPrecedant->pSuivant = pThread->pSuivant;
		pThread->pSuivant->pPrecedant = pThread->pPrecedant;
		// Désallouer sa pile
		free(pThread->ctx.uc_stack.ss_sp);
		// Retirer du ThreadTable
		gThreadTable[pThread->id] = NULL;
		// On passe au prochain thread à exécuter
		gpNextToExecuteInCircularBuffer = pThread->pSuivant;
		// Desallouer le TCB
		free(pThread);
	}
/*
	// On parcours tous les threads
	for (i = 0; i < gNumberOfThreadInCircularBuffer; i++) {
		TCB * pThread = gThreadTable[i];

		switch (pThread->etat) {
			case THREAD_EXECUTE:
				etat = 'E';
				break;
			case THREAD_PRET:
				etat = 'P';
				break;
			case THREAD_BLOQUE:
				etat = 'B';
				break;
			case THREAD_TERMINE:
				etat = 'T';
				break;
		}

		// Affichage de l'état du thread
		printf("ThreadID:%d État:%c\n", pThread->id, etat);

		// Si on a pas trouvé le prochain thread
		if (pThreadCourantNouveau->id ==  pThreadCourantAncien->id && pThread->etat == THREAD_PRET) {
			// On met le nouvel état et le thread courant
			pThreadCourantNouveau = pThread;
			if (pThreadCourantAncien->etat == THREAD_EXECUTE) {
				pThreadCourantAncien->etat = THREAD_PRET;
			}
			pThreadCourantNouveau->etat = THREAD_EXECUTE;
			gpThreadCourant = pThreadCourantNouveau;
		}

		// On prend le prochain thread pour l'autre itération
		pThread = pThread->pSuivant;
	}

	// On change le contexte
	swapcontext(&pThreadCourantAncien->ctx, &pThreadCourantNouveau->ctx);
	*/
}


/* ******************************************************************************************
                                   T h r e a d J o i n d r e
   ******************************************************************************************/
int ThreadJoindre(tid ThreadID){
	printf("\n  ******************************** ThreadJoindre(%d)  ******************************* \n",ThreadID);
	// Obtenir le thread par son tid
	TCB *threadAJoindre = gThreadTable[ThreadID];
	if (ThreadAJoindre == NULL || threadAJoindre->etat == THREAD_TERMINE) {
		return -1;
	}

	// Met le thread comme bloquer
	gpThreadCourant->etat = THREAD_BLOQUE;
	// Obtient l'espace nécessaire pour waitList
	WaitList *waitList = (struct WaitList *)malloc(sizeof(struct WaitList));
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
	gpThreadCourant->WakeupTime = time() + secondes;

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
