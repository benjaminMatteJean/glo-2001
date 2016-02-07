#include <string.h>
#include "sha256.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <pthread.h>


#define NB_THREADS 4


int flag = 0;

typedef struct {
    int startOffset;
    int endOffset;
    unsigned char * target;
}Params;

/* Pour calculer un intervalle de temps */
struct timespec diff(struct timespec start, struct timespec end);


void * search(void * args ) {
	Params *params = (Params *) args;
	FILE *pFichier;
	sha256_context ctx;
	char *pLigne;
	size_t longueur = 0;
	ssize_t read;
	unsigned char sha256sum[32];
	long positionFin;

	/* Ouverture du dictionnaire */
	pFichier = fopen("mots.txt","r");

	/* On va chercher la ligne de fin donné par endOffset */
	if (fseek(pFichier, params->endOffset, SEEK_SET) != 0) {
		printf("Problème lors du déplacement dans le fichier.\n");
	}
	positionFin = ftell(pFichier);

	if (fseek(pFichier, params->startOffset, SEEK_SET) != 0) {
		printf("Problème lors du déplacement dans le fichier.\n");
	}

	if (pFichier == NULL) {
		printf("Le dictionnaire mots.txt est manquant! \n");
		exit(EXIT_FAILURE);
	}

	pLigne = (char *)malloc(longueur);

	while ((read = getline(&pLigne, &longueur, pFichier)) != -1 && flag != 1 && ftell(pFichier) <= positionFin) {
		/* On enleve le retour de chariot '\n' a la fin */
		if (pLigne[read-1] == '\n') pLigne[read-1] = '\0';

		/* On initialise la fonction de hashing */
	    sha256_starts( &ctx );

	    /* On lui passe maintenant caractere par caractere le string a signer */
	    sha256_update( &ctx, (uint8 *)pLigne, strlen(pLigne));

		/* On termine le tout */
	    sha256_finish( &ctx, sha256sum );

	    /* On compare le string maintenant */
	    int Match = 1;
	    int j = 0;
	    for (j = 0; j < 32; j++ ) {
  			if (sha256sum[j]!=params->target[j]) {
  				Match = 0;
  				break;
  			}
	    }

	    if (Match) {
			printf("Bingo! Le mot est %s.\n",pLigne);
			flag = 1;
			break;
	    }
	}

	free(pLigne);
	fclose(pFichier);
	pthread_exit(NULL);
}

int main( int argc, char *argv[] )
{

    int i, j;
    char output[65];
    unsigned char sha256sumtarget[32];
    struct stat st;
    ssize_t read;
    Params params[NB_THREADS];
    pthread_t threads[NB_THREADS];


    read = strlen(argv[1]);
    if (read!=64) {
    	/* La signature en hexadecimal doit etre 64 caracteres */
      	printf("La signature %s n'a que %d caracteres\n!",argv[1],read);
    	exit(EXIT_FAILURE);
    }

    printf("La signature recherchee est %s\n",argv[1]);

  	char tmp[3];
  	tmp[2] = '\0'; /* Null-terminated string */
    /* Conversion du string en un entier de 32 octets */
    for (i=0;i<32; i++) {
    	tmp[0] = argv[1][2*i];
    	tmp[1] = argv[1][2*i+1];
		  sscanf(tmp,"%hhx",&sha256sumtarget[i]);
    }

    struct timespec time1, time2;
	  int temp;
	  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &time1);

    stat("mots.txt",&st);
    int size = st.st_size;
    int sizeSplitByThreads = size / NB_THREADS;
    int cpt = 0;
    for (i=0; i < NB_THREADS; i++) {
        params[i].startOffset = cpt;
        params[i].endOffset = cpt + sizeSplitByThreads - 1;
        params[i].target = sha256sumtarget;
        cpt += sizeSplitByThreads;

        if (i == NB_THREADS -1 ) {
            params[i].endOffset = size;
        }

        int status  = pthread_create(&threads[i], NULL,search,(void *)& params[i]);

        if( status != 0) {
            printf("Erreur, code : %d\n",status);
            exit(EXIT_FAILURE);
        }
    }

    for(i=0; i < NB_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }



	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &time2);
	struct timespec deltaTime = diff(time1,time2);
	printf("Temps de calcul : %ld.%ld seconde\n",deltaTime.tv_sec,deltaTime.tv_nsec);


    exit(EXIT_SUCCESS);
}

struct timespec diff(struct timespec start, struct timespec end)
{
	struct timespec temp;
	if ((end.tv_nsec-start.tv_nsec)<0) {
		temp.tv_sec = end.tv_sec-start.tv_sec-1;
		temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
	} else {
		temp.tv_sec = end.tv_sec-start.tv_sec;
		temp.tv_nsec = end.tv_nsec-start.tv_nsec;
	}
	return temp;
}
