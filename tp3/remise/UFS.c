#include "UFS.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "disque.h"

// Quelques fonctions qui pourraient vous être utiles
int NumberofDirEntry(int Size) {
	return Size/sizeof(DirEntry);
}

int min(int a, int b) {
	return a<b ? a : b;
}

int max(int a, int b) {
	return a>b ? a : b;
}

/* Cette fonction va extraire le repertoire d'une chemin d'acces complet, et le copier
   dans pDir.  Par exemple, si le chemin fourni pPath="/doc/tmp/a.txt", cette fonction va
   copier dans pDir le string "/doc/tmp" . Si le chemin fourni est pPath="/a.txt", la fonction
   va retourner pDir="/". Si le string fourni est pPath="/", cette fonction va retourner pDir="/".
   Cette fonction est calquée sur dirname, que je ne conseille pas d'utiliser car elle fait appel
   à des variables statiques/modifie le string entrant. Voir plus bas pour un exemple d'utilisation. */
int GetDirFromPath(const char *pPath, char *pDir) {
	strcpy(pDir,pPath);
	int len = strlen(pDir); // length, EXCLUDING null

	// On va a reculons, de la fin au debut
	while (pDir[len]!='/') {
		len--;
		if (len <0) {
			// Il n'y avait pas de slash dans le pathname
			return 0;
		}
	}
	if (len==0) {
		// Le fichier se trouve dans le root!
		pDir[0] = '/';
		pDir[1] = 0;
	}
	else {
		// On remplace le slash par une fin de chaine de caractere
		pDir[len] = '\0';
	}
	return 1;
}

/* Cette fonction va extraire le nom de fichier d'une chemin d'acces complet.
   Par exemple, si le chemin fourni pPath="/doc/tmp/a.txt", cette fonction va
   copier dans pFilename le string "a.txt" . La fonction retourne 1 si elle
   a trouvée le nom de fichier avec succes, et 0 autrement. Voir plus bas pour
   un exemple d'utilisation. */
int GetFilenameFromPath(const char *pPath, char *pFilename) {
	// Pour extraire le nom de fichier d'un path complet
	char *pStrippedFilename = strrchr(pPath,'/');
	if (pStrippedFilename!=NULL) {
		++pStrippedFilename; // On avance pour passer le slash
		if ((*pStrippedFilename) != '\0') {
			// On copie le nom de fichier trouve
			strcpy(pFilename, pStrippedFilename);
			return 1;
		}
	}
	return 0;
}


/* Un exemple d'utilisation des deux fonctions ci-dessus :
int bd_create(const char *pFilename) {
	char StringDir[256];
	char StringFilename[256];
	if (GetDirFromPath(pFilename, StringDir)==0) return 0;
	GetFilenameFromPath(pFilename, StringFilename);
	                  ...
*/


/* Cette fonction sert à afficher à l'écran le contenu d'une structure d'i-node */
void printiNode(iNodeEntry iNode) {
	printf("\t\t========= inode %d ===========\n",iNode.iNodeStat.st_ino);
	printf("\t\t  blocks:%d\n",iNode.iNodeStat.st_blocks);
	printf("\t\t  size:%d\n",iNode.iNodeStat.st_size);
	printf("\t\t  mode:0x%x\n",iNode.iNodeStat.st_mode);
	int index = 0;
	for (index =0; index < N_BLOCK_PER_INODE; index++) {
		printf("\t\t      Block[%d]=%d\n",index,iNode.Block[index]);
	}
}


/* ----------------------------------------------------------------------------------------
					            à vous de jouer, maintenant!
   ---------------------------------------------------------------------------------------- */


int bd_countfreeblocks(void) {
	char freeBlocksData[BLOCK_SIZE];
	int i, freeBlocksCount = 0;

	ReadBlock(FREE_BLOCK_BITMAP, freeBlocksData);

	for(i = 0; i < BLOCK_SIZE; i++) {
		if (freeBlocksData[i] != 0) {
			freeBlocksCount++;
		}
	}

	return freeBlocksCount;
}

int bd_stat(const char *pFilename, gstat *pStat) {

	return -1;
}

int bd_create(const char *pFilename) {
	return -1;
}

int bd_read(const char *pFilename, char *buffer, int offset, int numbytes) {
	return -1;
}

int bd_mkdir(const char *pDirName) {
	return -1;
}

int bd_write(const char *pFilename, const char *buffer, int offset, int numbytes) {
	return -1;
}

int bd_hardlink(const char *pPathExistant, const char *pPathNouveauLien) {
	return -1;
}

int bd_unlink(const char *pFilename) {
	return -1;
}

int bd_truncate(const char *pFilename, int NewSize) {
	return -1;
}

int bd_rmdir(const char *pFilename) {
	return -1;
}

int bd_rename(const char *pFilename, const char *pDestFilename) {
	return -1;
}

int bd_readdir(const char *pDirLocation, DirEntry **ppListeFichiers) {
	return -1;
}

int bd_symlink(const char *pPathExistant, const char *pPathNouveauLien) {
    return -1;
}

int bd_readlink(const char *pPathLien, char *pBuffer, int sizeBuffer) {
    return -1;
}

// Fonctions auxiliaires

int getINodeNumFromFilename(const char *pFilename) {
	// TODO Implémenter cette fonction (trouve l'inode correspondante au nom de fichier)
	return -1;
}

int seizeFreeBlock() {
	char freeBlocksData[BLOCK_SIZE];
	ReadBlock(FREE_BLOCK_BITMAP, freeBlocksData);
	int blockNum = BASE_BLOCK_INODE + (N_INODE_ON_DISK / NUM_INODE_PER_BLOCK);
	while (freeBlocksData[blockNum] == 0 && blockNum < N_BLOCK_ON_DISK) {
		blockNum++;
	}
	if (blockNum >= N_BLOCK_ON_DISK) {
		return -1;
	}
	freeBlocksData[blockNum] = 0;
	printf("GLOFS: Saisie bloc %d\n",blockNum);
	WriteBlock(FREE_BLOCK_BITMAP, freeBlocksData);
	return blockNum;
}

int releaseFreeBlock(UINT16 blockNum) {
	char freeBlocksData[BLOCK_SIZE];
	ReadBlock(FREE_BLOCK_BITMAP, freeBlocksData);
	freeBlocksData[blockNum] = 1;
	printf("GLOFS: Relache bloc %d\n",blockNum);
	WriteBlock(FREE_BLOCK_BITMAP, freeBlocksData);
	return 1;
}

int seizeFreeINode() {
	char freeINodeData[BLOCK_SIZE];
	ReadBlock(FREE_BLOCK_BITMAP, freeINodeData);
	int inodeNum = ROOT_INODE;
	while (freeINodeData[inodeNum] == 0 && inodeNum < N_INODE_ON_DISK) {
		inodeNum++;
	}
	if (inodeNum >= N_INODE_ON_DISK) {
		return -1;
	}
	freeINodeData[inodeNum] = 0;
	printf("GLOFS: Saisie i-node %d\n",inodeNum);
	WriteBlock(FREE_BLOCK_BITMAP, freeINodeData);
	return inodeNum;
}

int releaseFreeINode(UINT16 inodeNum) {
	char freeINodesData[BLOCK_SIZE];
	ReadBlock(FREE_INODE_BITMAP, freeINodesData);
	freeINodesData[inodeNum] = 1;
	printf("GLOFS: Relache i-node %d\n",inodeNum);
	WriteBlock(FREE_INODE_BITMAP, freeINodesData);
	return 1;
}
