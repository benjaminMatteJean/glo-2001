#include <string.h>
#include "sha256.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

/* Pour calculer un intervalle de temps */
struct timespec diff(struct timespec start, struct timespec end);

int main( int argc, char *argv[] )
{
    FILE *pFichier;
    int i, j;
    char output[65];
    sha256_context ctx;
    char *pLigne;
    unsigned char sha256sum[32];
    unsigned char sha256sumtarget[32];
    size_t longueur = 0;
    ssize_t read;

    
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
      
    /* Ouverture du dictionnaire */
    pFichier = fopen("mots.txt","r");
    
    if (pFichier == NULL) {
    	printf("Le dictionnaire mots.txt est manquant\n!");
    	exit(EXIT_FAILURE);
    }
    
    longueur = 1000;
    pLigne = (char *)malloc(longueur);
    
    
    struct timespec time1, time2;
	int temp;
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &time1);
    
    while ((read = getline(&pLigne, &longueur, pFichier)) != -1) {

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
    	for (j = 0; j < 32; j++ ) {
    		if (sha256sum[j]!=sha256sumtarget[j]) {
    			Match = 0;
    			break;
    		}
    	}
    	
    	if (Match) {
    		printf("Bingo! Le mot est %s.\n",pLigne);
    		break;
    	}

	}

	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &time2);
	struct timespec deltaTime = diff(time1,time2);
	printf("Temps de calcul : %ld.%ld\n seconde",deltaTime.tv_sec,deltaTime.tv_nsec);

	free(pLigne);
	fclose(pFichier);
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

