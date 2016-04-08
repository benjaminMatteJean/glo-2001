#include "UFS.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
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

// FONCTIONS AUXILIAIRES

/* Retourne le numero d'inode du fichier pFileName dans le répertoire associé au numero d'inode parentINodeNum */
int getFileINodeNumFromParent(const char *pFileName, int parentINodeNum) {
	if (strcmp(pFileName, "") == 0) return parentINodeNum;
	char blockData[BLOCK_SIZE];
	// On trouve le numero du block d'i-nodes qui contient le numero d'i-node parent
	int iNodesBlockNum = BASE_BLOCK_INODE + (parentINodeNum / NUM_INODE_PER_BLOCK);
	// Lecture du block d'i-nodes
	ReadBlock(iNodesBlockNum, blockData);
	iNodeEntry *pINodes = (iNodeEntry *) blockData;
	// On trouve la position de l'i-node parent dans le block d'i-nodes
	UINT16 iNodePosition = parentINodeNum % NUM_INODE_PER_BLOCK;
	// On trouve le nombre d'entrées dans le block de l'i-node parent
	UINT16 entryNum = NumberofDirEntry(pINodes[iNodePosition].iNodeStat.st_size);
	// Lecture du block de données associé à l'i-node parent
	ReadBlock(pINodes[iNodePosition].Block[0], blockData);
	DirEntry *pDE = (DirEntry *) blockData;
	// Pour chaque entrée du block (sauf . et ..) on vérifie le nom de fichier
	size_t n;
	for (n = 0; n < entryNum; n++) {
		if (strcmp(pFileName, pDE[n].Filename) == 0) {
			return pDE[n].iNode;	// On a trouvé le numéro d'i-node correspondant au nom de fichier/repertoire
		}
	}
	return -1;	// Le nom de fichier/répertoire n'existe pas
}

/* Fait une récursion sur le path pPath et retourne le numéro d'inode du fichier pFilename */
int getInode(const char *pPath, const char *pFilename, int parentINodeNum) {
	if (parentINodeNum == -1) return -1;

	char pName[FILENAME_SIZE];
	int iCar, iSlash = 0;
	for (iCar = 0; iCar < FILENAME_SIZE; iCar++) {
		if (pPath[iCar] == 0) break;
		else if (pPath[iCar] == '/' && iCar != 0) break;
		else if (pPath[iCar] == '/') iSlash++;
		else {
			pName[(iCar-iSlash)] = pPath[iCar];
		}
	}
	pName[iCar - iSlash] = 0;
	if (strcmp(pFilename, pName) == 0) {
		return getFileINodeNumFromParent(pName, parentINodeNum);
	} else {
		getInode(pPath + strlen(pName) + 1, pFilename, getFileINodeNumFromParent(pName, parentINodeNum));
	}
}

/* Retourne le numero d'inode correspondant au fichier spécifié par le path */
int getFileINodeNumFromPath(const char *pPath) {
	if (strcmp(pPath, "/") == 0) return ROOT_INODE;
	char pName[FILENAME_SIZE];
	if (GetFilenameFromPath(pPath, pName) == 0) pName[0] = 0;
	return getInode(pPath, pName, ROOT_INODE);
}

/* Assigne un inodeEntry correspondant au numero d'inode iNodeNum au pointeur pIE */
int getINodeEntry(ino iNodeNum, iNodeEntry *pIE) {
	if (iNodeNum > N_INODE_ON_DISK || iNodeNum < 0) return -1;
	char blockData[BLOCK_SIZE];
	// On trouve le numero du block d'i-nodes qui contient le numero d'i-node
	UINT16 iNodesBlockNum = BASE_BLOCK_INODE + (iNodeNum / NUM_INODE_PER_BLOCK);
	// On trouve la position de l'i-node dans le block d'i-node
	UINT16 iNodePosition = iNodeNum % NUM_INODE_PER_BLOCK;
	// Lecture du block d'i-nodes
	ReadBlock(iNodesBlockNum, blockData);
	iNodeEntry *pINodes = (iNodeEntry *) blockData;
	*pIE = pINodes[iNodePosition];
	return 0;
}

/* Saisi un block libre, corrige la valeur du bitmap des blockslibres
Retourne le numero du block saisi */
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

/* Relâche un block, corrige la valeur du bitmap des blockslibres */
int releaseFreeBlock(UINT16 blockNum) {
	char freeBlocksData[BLOCK_SIZE];
	ReadBlock(FREE_BLOCK_BITMAP, freeBlocksData);
	freeBlocksData[blockNum] = 1;
	printf("GLOFS: Relache bloc %d\n",blockNum);
	WriteBlock(FREE_BLOCK_BITMAP, freeBlocksData);
	return 1;
}

/* Saisi une inode, corrige la valeur du bitmap des inodeslibres
	Retourne le numero d'inode de l'inode saisie */
int seizeFreeINode() {
   	char freeINodesData[BLOCK_SIZE];
   	ReadBlock(FREE_INODE_BITMAP, freeINodesData);
   	int inodeNum = ROOT_INODE;
   	while (freeINodesData[inodeNum] == 0 && inodeNum < N_INODE_ON_DISK) {
   		inodeNum++;
   	}
   	if (inodeNum >= N_INODE_ON_DISK) {
   		return -1;
   	}
   	freeINodesData[inodeNum] = 0;
   	printf("GLOFS: Saisie i-node %d\n",inodeNum);
   	WriteBlock(FREE_INODE_BITMAP, freeINodesData);
   	return inodeNum;

}

/* Relâche une inode, corrige la valeur du bitmap des inodeslibres */
int releaseFreeINode(UINT16 inodeNum) {
   	char freeINodesData[BLOCK_SIZE];
   	ReadBlock(FREE_INODE_BITMAP, freeINodesData);
   	freeINodesData[inodeNum] = 1;
   	printf("GLOFS: Relache i-node %d\n",inodeNum);
   	WriteBlock(FREE_INODE_BITMAP, freeINodesData);
   	return 1;
}

/* Prend un pointeur de iNodeEntry et écrit dans l'image (met à jour) l'inode */
void writeINodeOnDisk(iNodeEntry *pIE) {
	char blockData[BLOCK_SIZE];
	UINT16 iNodesBlockNum = BASE_BLOCK_INODE + (pIE->iNodeStat.st_ino / NUM_INODE_PER_BLOCK);
	UINT16 iNodePosition = pIE->iNodeStat.st_ino % NUM_INODE_PER_BLOCK;
	ReadBlock(iNodesBlockNum, blockData);
	iNodeEntry *pINodes = (iNodeEntry *) blockData;
	pINodes[iNodePosition] = *pIE;
	WriteBlock(iNodesBlockNum, blockData);
}


void addDirEntryInDir(iNodeEntry * destDirInode, ino inodeNum, char * filename) {
	DirEntry *pDirEntry;
	char block[BLOCK_SIZE];


	destDirInode->iNodeStat.st_size += sizeof(DirEntry);
	writeINodeOnDisk(destDirInode);

	UINT16 inodeSize = destDirInode->iNodeStat.st_size;
	int nbDirEntry = NumberofDirEntry(inodeSize);
	UINT16 blockNum = destDirInode->Block[0];

	ReadBlock(blockNum, block);
	pDirEntry = (DirEntry *) block;

	pDirEntry += nbDirEntry - 1;
	pDirEntry->iNode = inodeNum;
	strcpy(pDirEntry->Filename, filename);

	WriteBlock(blockNum, block);

}

void removeDirEntryInDir(iNodeEntry * iNodeDir , ino inoToDelete) {
	UINT16 size_inode = iNodeDir->iNodeStat.st_size;
	iNodeDir->iNodeStat.st_size -= BLOCK_SIZE / sizeof(DirEntry);

	writeINodeOnDisk(iNodeDir);

	UINT16 blockNum = iNodeDir->Block[0];
	char block[BLOCK_SIZE];
	ReadBlock(blockNum, block);

	DirEntry * pDirEntry = (DirEntry *) block;
	int findIndicator = 0;
	int i = 0;
	int count = NumberofDirEntry(size_inode);
	while (i < count) {
		if (pDirEntry[i].iNode == inoToDelete) {
			findIndicator = 1;
		}
		if (findIndicator == 1) {
			pDirEntry[i] = pDirEntry[i + 1];
		}
		i++;
	}
	for (i = 0; i < count; i++) {

	}
	WriteBlock(blockNum, block);
}

// FIN FONCTIONS AUXILIAIRES

int bd_countfreeblocks(void) {
	char freeBlocksData[BLOCK_SIZE];
	int i, freeBlocksCount = 0;
	ReadBlock(FREE_BLOCK_BITMAP, freeBlocksData);
	for(i = 0; i < N_BLOCK_ON_DISK; i++) {
		if (freeBlocksData[i] != 0) {
			freeBlocksCount++;
		}
	}
	return freeBlocksCount;
}

int bd_stat(const char *pFilename, gstat *pStat) {
	ino iNodeNum = getFileINodeNumFromPath(pFilename);
	iNodeEntry iNode;
	if (iNodeNum == -1) return -1;	// Le fichier/répertoire est inexistant
	if (getINodeEntry(iNodeNum, &iNode) != 0) return -1; // Le fichier/répertoire est inexistant
	*pStat = iNode.iNodeStat;
	return 0; // En cas de succès
}

int bd_create(const char *pFilename) {
	char strDirectory[BLOCK_SIZE];
	char strFile[FILENAME_SIZE];
	ino dirInode , fileInode = 0;

	GetDirFromPath(pFilename, strDirectory);
	GetFilenameFromPath(pFilename, strFile);
	dirInode = getFileINodeNumFromPath(strDirectory);
	if (dirInode == -1) {
		return -1;
	}

	fileInode = getFileINodeNumFromPath(strFile);
	if(fileInode != -1) {
		return -2;
	}

	fileInode = seizeFreeINode();
	iNodeEntry  pINode;
	getINodeEntry(fileInode, &pINode);
	pINode.iNodeStat.st_mode = G_IFREG;
	pINode.iNodeStat.st_size = 0;
	pINode.iNodeStat.st_ino = fileInode;
	pINode.iNodeStat.st_nlink = 1;
	pINode.iNodeStat.st_blocks = 0;
	pINode.iNodeStat.st_mode = pINode.iNodeStat.st_mode | G_IRWXU | G_IRWXG;

	writeINodeOnDisk(&pINode);

	iNodeEntry pInodeDir;
	getINodeEntry(dirInode, &pInodeDir);

	addDirEntryInDir(&pInodeDir, fileInode, strFile);

	return 0; // En cas de succès
}

int bd_read(const char *pFilename, char *buffer, int offset, int numbytes) {
	ino iNodeNum = getFileINodeNumFromPath(pFilename);
	iNodeEntry iNode;

	if (iNodeNum == -1) return -1;							// Le fichier pFilename est inexistant
	if (getINodeEntry(iNodeNum, &iNode) != 0) return -1;	// Le fichier pFilename est inexistant
	if (iNode.iNodeStat.st_mode & G_IFDIR) return -2; 		// Le fichier pFilename est un répertoire
	if (iNode.iNodeStat.st_size <= offset) return 0; 		// L'offset engendre un overflow

	char fileDataBlock[BLOCK_SIZE];
	ReadBlock(iNode.Block[0], fileDataBlock);
	int i = 0, octets = 0;
	for (i = offset; i < iNode.iNodeStat.st_size && i < (offset + numbytes); i++) {
		buffer[octets] = fileDataBlock[i];
		octets++;
	}
	return octets; // retourne le nombre d'octets lus
}

int bd_mkdir(const char *pDirName) {
	char dirName[FILENAME_SIZE];
	char filename[FILENAME_SIZE];

	ino dirNameIno = getFileINodeNumFromPath(pDirName);
	if(dirNameIno != -1) return -2; // Il existe déjà.

	if(GetDirFromPath(pDirName, dirName) == 0) return -1;
	if(GetFilenameFromPath(pDirName,filename) == 0) return -1;


	ino parentDirIno = getFileINodeNumFromPath(dirName);
	if (parentDirIno == -1) return -1;
	iNodeEntry parentInode;

	if(getINodeEntry(parentDirIno, &parentInode) != 0) return -1;

	if(parentInode.iNodeStat.st_mode & G_IFREG) return -1;

	iNodeEntry newInode;
	ino newInodeIno = seizeFreeINode();
	if(newInodeIno == -1) return -1;
	getINodeEntry(newInodeIno, &newInode);
	int blockNumber = seizeFreeBlock();
	if(blockNumber == -1) return -1;

	parentInode.iNodeStat.st_nlink++;
	writeINodeOnDisk(&parentInode);
	addDirEntryInDir(&parentInode, newInodeIno, filename);

	getINodeEntry(newInodeIno, &newInode);
	newInode.Block[0] = blockNumber;
	newInode.iNodeStat.st_ino = newInodeIno;
	newInode.iNodeStat.st_mode = G_IFDIR | G_IRWXU | G_IRWXG;
	newInode.iNodeStat.st_nlink = 2;
	newInode.iNodeStat.st_size = sizeof(DirEntry) * 2;
	newInode.iNodeStat.st_blocks = 1;
	writeINodeOnDisk(&newInode);

	// On remplie le block
	char block[BLOCK_SIZE];
	ReadBlock(blockNumber, block);
	DirEntry * pDirEntry = (DirEntry *) block;
	pDirEntry->iNode = newInodeIno;
	strcpy(pDirEntry->Filename, ".");
	pDirEntry++;
	pDirEntry->iNode = parentDirIno;
	strcpy(pDirEntry->Filename, "..");
	WriteBlock(blockNumber, block);

	return 0; // En cas de succès
}

int bd_write(const char *pFilename, const char *buffer, int offset, int numbytes) {
	iNodeEntry fileInode;
	ino filenameIno = getFileINodeNumFromPath(pFilename);

	if(filenameIno == -1) return -1; // Il n'existe pas.
	if(getINodeEntry(filenameIno, &fileInode) != 0) return -1;
	if(fileInode.iNodeStat.st_mode & G_IFDIR) return -2;
	if(fileInode.iNodeStat.st_size < offset &&  offset >= N_BLOCK_PER_INODE*BLOCK_SIZE) return -4;
	if(fileInode.iNodeStat.st_size < offset) return -3;
	if(fileInode.iNodeStat.st_size == 0 && numbytes > 0 && fileInode.iNodeStat.st_blocks == 0) {
		int blockNum = seizeFreeBlock();
		if(blockNum == -1) return 0; // On fait quoi avec plus de block disponible.
		fileInode.Block[0] = blockNum;
		fileInode.iNodeStat.st_blocks += 1;
	}

	char fileDataBlock[BLOCK_SIZE];
	char newBuffer[BLOCK_SIZE];
	ReadBlock(fileInode.Block[0], fileDataBlock);
	int i = 0, octets = 0 , cpt = 0;

	for (i = 0; i < fileInode.iNodeStat.st_size; i++) {
		newBuffer[i] = fileDataBlock[i];
	}

	for (i = offset; i < (offset + numbytes) && i <= BLOCK_SIZE && cpt <= numbytes; i++) {
		if(newBuffer[i] != buffer[cpt]) {
			newBuffer[i] = buffer[cpt];
			octets++;
		}
		cpt++;
	}

	WriteBlock(fileInode.Block[0] , newBuffer);
	if(offset + octets > fileInode.iNodeStat.st_size) {
		fileInode.iNodeStat.st_size = offset + octets;
	}

	writeINodeOnDisk(&fileInode);

	return octets;
}

int bd_hardlink(const char *pPathExistant, const char *pPathNouveauLien) {
	char dirNameNouveaulien[256];
	GetDirFromPath(pPathNouveauLien, dirNameNouveaulien);

	ino ino_existant = getFileINodeNumFromPath(pPathExistant);
	ino ino_dirNouveauLien = getFileINodeNumFromPath(dirNameNouveaulien);
	iNodeEntry IE_existant;
	iNodeEntry IE_dirNouveauLien;

	if (ino_existant == -1 || ino_dirNouveauLien == -1) return -1;	// Le fichier pPathExistant ou pPathNouveauLien est inexistant
	if (getFileINodeNumFromPath(pPathNouveauLien) != -1) return -2;	// le fichier pPathNouveauLien existe déjà
	if (getINodeEntry(ino_existant, &IE_existant) != 0) return -1;	// Le fichier pPathExistant est inexistant
	if (getINodeEntry(ino_dirNouveauLien, &IE_dirNouveauLien) != 0) return -1; // Le répertoire qui va contenir le lien spécifié par pPathNouveauLien est inexistant
	if (IE_existant.iNodeStat.st_mode & G_IFDIR) return -3; 		// Le fichier pPathExistant est un répertoire

	char blockData[BLOCK_SIZE], linkName[FILENAME_SIZE];
	GetFilenameFromPath(pPathNouveauLien, linkName);
	ReadBlock(IE_dirNouveauLien.Block[0], blockData);
	DirEntry *pDirEntries = (DirEntry *) blockData;
	UINT16 entryNum = NumberofDirEntry(IE_dirNouveauLien.iNodeStat.st_size);
	pDirEntries[entryNum].iNode = IE_existant.iNodeStat.st_ino;
	strcpy(pDirEntries[entryNum].Filename, linkName);

	IE_dirNouveauLien.iNodeStat.st_size += sizeof(DirEntry);
	IE_existant.iNodeStat.st_nlink++;

	WriteBlock(IE_dirNouveauLien.Block[0], blockData);
	writeINodeOnDisk(&IE_existant);
	writeINodeOnDisk(&IE_dirNouveauLien);

	return 0; // En cas de succès
}

int bd_unlink(const char *pFilename) {
	char dirName[256], fileName[FILENAME_SIZE];
	GetDirFromPath(pFilename, dirName);
	GetFilenameFromPath(pFilename, fileName);

	ino fileINodeNum = getFileINodeNumFromPath(pFilename);
	ino dirINodeNum = getFileINodeNumFromPath(dirName);
	iNodeEntry IE_file;
	iNodeEntry IE_dir;

	if (fileINodeNum == -1 || dirINodeNum == -1) return -1; 	// Si pFilename ou dirName n'existe pas
	if (getINodeEntry(fileINodeNum, &IE_file) != 0) return -1;	// Le fichier pFilename est inexistant
	if (getINodeEntry(dirINodeNum, &IE_dir) != 0) return -1;	// Le répertoire qui contient le fichier pFilename est inexistant
	if (IE_file.iNodeStat.st_mode & G_IFDIR) return -2; 		// Le fichier n'est pas un fichier régulier G_IFREG ni un lien symbolique G_IFLNK

	char blockData[BLOCK_SIZE];
	ReadBlock(IE_dir.Block[0], blockData);
	DirEntry *pDirEntries = (DirEntry *) blockData;
	UINT16 entryNum = NumberofDirEntry(IE_dir.iNodeStat.st_size);
	size_t iEntry;
	size_t iNext;
	for (iEntry = 0; iEntry < entryNum; iEntry++) {
		if (strcmp(pDirEntries[iEntry].Filename, fileName) == 0) {
			if (iEntry != entryNum - 1) {
				for (iNext = 1; iNext < entryNum - iEntry; iNext++) {
					pDirEntries[iEntry + iNext - 1] = pDirEntries[iEntry + iNext];
				}
				break;
			}
		}
	}
	WriteBlock(IE_dir.Block[0], blockData);

	IE_dir.iNodeStat.st_size -= sizeof(DirEntry);
	writeINodeOnDisk(&IE_dir);

	IE_file.iNodeStat.st_nlink--;
	if (IE_file.iNodeStat.st_nlink == 0) {
		if (IE_file.iNodeStat.st_blocks > 0) releaseFreeBlock(IE_file.Block[0]);
		releaseFreeINode(fileINodeNum);
	} else {
		writeINodeOnDisk(&IE_file);
	}

	return 0; // En cas de succès
}

int bd_truncate(const char *pFilename, int NewSize) {
	iNodeEntry filenameInode;
	ino filenameIno = getFileINodeNumFromPath(pFilename);
	int returnSize;
	if (filenameIno == -1) return -1;							// Le fichier pFilename est inexistant
	if (getINodeEntry(filenameIno, &filenameInode) != 0) return -1;	// Le fichier pFilename est inexistant
	if (filenameInode.iNodeStat.st_mode & G_IFDIR) return -2; 		// Le fichier pFilename est un répertoire

	if(filenameInode.iNodeStat.st_size < NewSize) return filenameInode.iNodeStat.st_size;

	if(NewSize == 0) releaseFreeBlock(filenameInode.Block[0]);
	filenameInode.iNodeStat.st_size = NewSize;
	returnSize = filenameInode.iNodeStat.st_size;
	writeINodeOnDisk(&filenameInode);

	return returnSize;
}

int bd_rmdir(const char *pFilename) {

	char dirName[256], fileName[FILENAME_SIZE];
	GetDirFromPath(pFilename, dirName);
	GetFilenameFromPath(pFilename, fileName);

	ino fileINodeNum = getFileINodeNumFromPath(pFilename);
	ino dirINodeNum = getFileINodeNumFromPath(dirName);

	if (fileINodeNum == -1 || dirINodeNum == -1) return -1;

	iNodeEntry filenameInode , dirInode;

	if(getINodeEntry(fileINodeNum, &filenameInode) != 0) return -1;
	if(getINodeEntry(dirINodeNum, &dirInode)) return -1;

	if(filenameInode.iNodeStat.st_mode & G_IFREG) return -2;

	if(NumberofDirEntry(filenameInode.iNodeStat.st_size) > 2) return -3;

	removeDirEntryInDir(&dirInode, fileINodeNum);

	dirInode.iNodeStat.st_nlink--;
	writeINodeOnDisk(&dirInode);
	releaseFreeBlock(filenameInode.Block[0]);
	releaseFreeINode(fileINodeNum);

	return 0; // En cas de succès
}

int bd_rename(const char *pFilename, const char *pDestFilename) {

	// Succès s'il s'agit du mêmem fichier dans le même répertoire.
	if(strcmp(pFilename, pDestFilename) ==0) {
		return 0;
	}

	int state = bd_hardlink(pFilename, pDestFilename);

	if (state == 0) {
		bd_unlink(pFilename);
		return 0;
	} else if (state == -2 || state == -1) {
		return -1;
	} else {
		// Il s'agit d'un répertoire
		char directorySource[BLOCK_SIZE];
		char directoryDest[BLOCK_SIZE];
		char filename[BLOCK_SIZE];
		iNodeEntry sourceDirInode;
		iNodeEntry destDirInode;
		iNodeEntry filenameInode;


		ino filenameIno = getFileINodeNumFromPath(pFilename);
		if (filenameIno == -1) return -1;

		if(GetDirFromPath(pFilename, directorySource) == 0) return -1;
		if(GetDirFromPath(pDestFilename, directoryDest) == 0) return -1;
		if(GetFilenameFromPath(pDestFilename, filename) == 0) return -1;

		ino directorySourceIno = getFileINodeNumFromPath(directorySource);
		if(directorySourceIno == -1) return -1;

		ino destFilenameIno = getFileINodeNumFromPath(pDestFilename);
		if (destFilenameIno != -1) return -1;

		destFilenameIno = getFileINodeNumFromPath(directoryDest);
		if(destFilenameIno == -1) return -1;

		if(getINodeEntry(directorySourceIno, &sourceDirInode) != 0) return -1;
		removeDirEntryInDir(&sourceDirInode, filenameIno);

		// Décrémenter le nombre de link
		if(getINodeEntry(directorySourceIno, &sourceDirInode) != 0) return -1;
		sourceDirInode.iNodeStat.st_nlink--;
		writeINodeOnDisk(&sourceDirInode);

		if(getINodeEntry(destFilenameIno, &destDirInode) != 0) return -1;
		addDirEntryInDir(&destDirInode,filenameIno,filename);

		// Augmenter le  nb de link
		if(getINodeEntry(destFilenameIno, &destDirInode) != 0) return -1;
		destDirInode.iNodeStat.st_nlink++;
		writeINodeOnDisk(&destDirInode);

		if(getINodeEntry(filenameIno, &filenameInode) !=  0) return -1;
		char block[BLOCK_SIZE];
		UINT16 blockNum = filenameInode.Block[0];
		ReadBlock(blockNum, block);
		DirEntry * pDirEntry = (DirEntry *) block;
		pDirEntry++;
		pDirEntry->iNode = destFilenameIno;
		WriteBlock(blockNum,block);

		return 0;
	}
}

int bd_readdir(const char *pDirLocation, DirEntry **ppListeFichiers) {
	ino iNodeNum = getFileINodeNumFromPath(pDirLocation);
	iNodeEntry iNode;

	if (iNodeNum == -1) return -1;							// Le fichier pDirLocation est inexistant
	if (getINodeEntry(iNodeNum, &iNode) != 0)  return -1; 	// Le fichier pDirLocation est inexistant
	if (!(iNode.iNodeStat.st_mode & G_IFDIR)) return -1; 	// Le fichier pDirLocation n'est pas un répertoire

	char fileDataBlock[BLOCK_SIZE];
	ReadBlock(iNode.Block[0], fileDataBlock);

	*ppListeFichiers = (DirEntry*) malloc(iNode.iNodeStat.st_size);
	memcpy((*ppListeFichiers), fileDataBlock, iNode.iNodeStat.st_size);

	UINT16 entryNum = NumberofDirEntry(iNode.iNodeStat.st_size);

	return entryNum; // Retourne le nombre d'entrées du répertoire
}

int bd_symlink(const char *pPathExistant, const char *pPathNouveauLien) {
	ino inoFile = getFileINodeNumFromPath(pPathExistant);
	char destinationDir[BLOCK_SIZE];
	char filenameDest[BLOCK_SIZE];
	iNodeEntry newSymlinkInode;
	iNodeEntry symlinkFileInode;

	if(GetDirFromPath(pPathNouveauLien, destinationDir) == 0) return -1;
	if(GetFilenameFromPath(pPathNouveauLien, filenameDest) == 0) return -1;

	ino newSymlinkIno = getFileINodeNumFromPath(pPathNouveauLien);
	if(getINodeEntry(newSymlinkIno, &newSymlinkInode) == 0) return -2;

	bd_create(pPathNouveauLien);
	newSymlinkIno = getFileINodeNumFromPath(pPathNouveauLien);
	if(getINodeEntry(newSymlinkIno, &newSymlinkInode) != 0) return -1;

	getINodeEntry(inoFile, &symlinkFileInode);

	newSymlinkInode.iNodeStat.st_mode |= G_IFLNK | G_IFREG;
	writeINodeOnDisk(&newSymlinkInode);

 	bd_write(pPathNouveauLien, pPathExistant,0,strlen(pPathExistant)+1);

	return 0; // En cas de succès
}

int bd_readlink(const char *pPathLien, char *pBuffer, int sizeBuffer) {
	ino iNodeNum = getFileINodeNumFromPath(pPathLien);
	iNodeEntry iNode;

	if (iNodeNum == -1) return -1;							// Le fichier pPathLien est inexistant
	if (getINodeEntry(iNodeNum, &iNode) != 0)  return -1; 	// Le fichier pPathLien est inexistant
	if (!(iNode.iNodeStat.st_mode & G_IFREG)) return -1; 	// Le fichier pPathLien n'est pas un lien symbolique
	if (!(iNode.iNodeStat.st_mode & G_IFLNK)) return -1; 	// Le fichier pPathLien n'est pas un lien symbolique

	char fileDataBlock[BLOCK_SIZE];
	ReadBlock(iNode.Block[0], fileDataBlock);
	int iCar = 0;
	for (iCar = 0; iCar < iNode.iNodeStat.st_size && iCar < sizeBuffer; iCar++) {
		pBuffer[iCar] = fileDataBlock[iCar];
	}

	return iCar; // Le nombre de caractères lus;
}
