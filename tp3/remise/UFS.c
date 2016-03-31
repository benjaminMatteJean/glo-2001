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
	for (size_t n = 0; n < entryNum; n++) {
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
   	ReadBlock(FREE_BLOCK_BITMAP, freeINodesData);
   	int inodeNum = ROOT_INODE;
   	while (freeINodesData[inodeNum] == 0 && inodeNum < N_INODE_ON_DISK) {
   		inodeNum++;
   	}
   	if (inodeNum >= N_INODE_ON_DISK) {
   		return -1;
   	}
   	freeINodesData[inodeNum] = 0;
   	printf("GLOFS: Saisie i-node %d\n",inodeNum);
   	WriteBlock(FREE_BLOCK_BITMAP, freeINodesData);
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

/* Prend un pointeur de iNodeEntry et écrit dans l'image (met à jour) ses statistiques */
void updateINodeStats(iNodeEntry *pIE) {
	char blockData[BLOCK_SIZE];
	UINT16 iNodesBlockNum = BASE_BLOCK_INODE + (pIE->iNodeStat.st_ino / NUM_INODE_PER_BLOCK);
	UINT16 iNodePosition = pIE->iNodeStat.st_ino % NUM_INODE_PER_BLOCK;
	ReadBlock(iNodesBlockNum, blockData);
	iNodeEntry *pINodes = (iNodeEntry *) blockData;
	pINodes[iNodePosition].iNodeStat = pIE->iNodeStat;
	WriteBlock(iNodesBlockNum, blockData);
}

// FIN FONCTIONS AUXILIAIRES

/* Cette fonction retourne le nombre de bloc de données libres sur le disque. */
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

/* Cette fonction copie les métadonnées gstat du fichier pFilename vers le pointeur pStat . Les
métadonnées du fichier pFilename doivent demeurer inchangées. La fonction retourne -1 si le fichier
pFilename est inexistant. Autrement, la fonction retourne 0. */
int bd_stat(const char *pFilename, gstat *pStat) {
	// On trouve le numero d'i-node correspondant au nom de fichier à partir de la racine
	ino iNodeNum = getFileINodeNumFromPath(pFilename);
	iNodeEntry iNode;
	if (iNodeNum == -1) return -1;	// Le fichier/répertoire est inexistant
	if (getINodeEntry(iNodeNum, &iNode) != 0) return -1; // Le fichier/répertoire est inexistant
	// Copie des métadonnées gstat du fichier vers le pointeur pStat
	*pStat = iNode.iNodeStat;
	return 0; // En cas de succès
}

/* Cette fonction vient créer un fichier normal vide (bit G_IFREG de st_mode à 1, taille=0, donc sans
bloc de données) avec le nom et à l’endroit spécifié par le chemin d’accès pFilename . Par exemple, si
pFilename est égal à /doc/tmp/a.txt , vous devez créer le fichier a.txt dans le répertoire
/doc/tmp . Si ce répertoire n’existe pas, retournez -1. Assurez-vous aussi que ce fichier n’existe pas
déjà, auquel cas retournez -2. Pour les permissions rwx, simplement les mettre toutes à 1, en faisant
st_mode|=G_IRWXU|G_IRWXG . Retournez 0 pour indiquer le succès de l’opération. */
int bd_create(const char *pFilename) {
	// TODO à compléter
	return -1; // Si le répertoire n'existe pas
	return -2; // Si le fichier existe déjà
	return 0; // En cas de succès
}

/* Cette fonction va lire numbytes octets dans le fichier pFilename , à partir de la position offset , et les
copier dans buffer . La valeur retournée par la fonction est le nombre d’octets lus. Par exemple, si le
fichier fait 100 octets et que vous faites une lecture pour offset =10 et numbytes =200, la valeur
retournée par bd_read sera 90. Si le fichier pFilename est inexistant, la fonction devra retourner -1. Si
le fichier pFilename est un répertoire, la fonction devra retourner -2. Si l’ offset fait en sorte qu’il
dépasse la taille du fichier, cette fonction devra simplement retourner 0, car vous ne pouvez pas lire
aucun caractère. Notez que le nombre de blocs par fichier est limité à 1, ce qui devrait simplifier le code
de lecture. */
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

/* Cette fonction doit créer le répertoire pDirName. Si le chemin d’accès à pDirName est inexistant, ne
faites rien et retournez -1, par exemple si on demande de créer le répertoire /doc/tmp/toto et que le
répertoire /doc/tmp n’existe pas. Assurez-vous que l’i-node correspondant au répertoire est marqué
comme répertoire (bit G_IFDIR de st_mode à 1). Si le répertoire pDirName existe déjà, retournez
avec -2. Pour les permissions rxw, simplement les mettre toutes à 1, en faisant
st_mode|=G_IRWXU|G_IRWXG . Assurez-vous aussi que le répertoire contiennent les deux répertoires
suivants : « . » et « .. ». N’oubliez-pas d’incrémenter st_nlink pour le répertoire actuel « . » et parent
« .. ». En cas de succès, retournez 0. */
int bd_mkdir(const char *pDirName) {
	// TODO à compléter
	return -1; // Si le chemin d'accès pDirName est inexistant
	return -2; // Si le répertoire pDirName existe déjà
	return 0; // En cas de succès
}

/* Cette fonction va écrire numbytes octets du buffer dans le fichier pFilename , à partir de la position
offset (0 indique le début du fichier). La valeur retournée par la fonction est le nombre d’octets écrits.
Vous devez vérifier que :
 Le fichier pFilename existe. Dans le cas contraire, cette fonction devra retourner -1.
 Le fichier pFilename n’est pas un répertoire. Dans le cas contraire, cette fonction devra
retourner -2.
 L’ offset doit être plus petit ou égal à la taille du fichier. Dans le cas contraire, cette fonction
devra retourner -3. Par contre, si l’ offset est plus grand ou égal à la taille maximale supportée
par ce système de fichier, retournez la valeur -4.
 La taille finale ne doit pas dépasser la taille maximale d’un fichier sur ce mini-UFS, qui est dicté
par le nombre de bloc d’adressage direct N_BLOCK_PER_INODE . et la taille d’un bloc. Vous
devez quand même écrire le plus possible dans le fichier, jusqu’à atteindre cette taille limite. La
fonction retournera ce nombre d’octet écrit.
N’oubliez-pas de modifier la taille du fichier st_size . */
int bd_write(const char *pFilename, const char *buffer, int offset, int numbytes) {
	// TODO à compléter
	return -1; // Si le fichier pFilename est inexistant
	return -2; // Si pFilename est un répertoire
	return -3; // Si l'offset est supérieur ou égal à la taille du fichier (engendre un overflow)
	return -4; // Si l'offset est supérieur ou égal à la taille maximale supportée
	// return le nombre d’octets écrits;
}

/* Cette fonction créer un hardlink entre l’i-node du fichier pPathExistant et le nom de fichier
pPathNouveauLien . Assurez-vous que le fichier original pPathExistant n’est pas un répertoire
(bit G_IFDIR de st_mode à 0 et bit G_IFREG à 1), auquel cas retournez -3. Assurez-vous aussi que
le répertoire qui va contenir le lien spécifié dans pPathNouveauLien existe, sinon retournez -1.
N’oubliez-pas d’incrémenter la valeur du champ st_nlink dans l’i-node. Assurez-vous que la
commande fonctionne aussi si le lien est créé dans le même répertoire, i.e.
bd_hardlink("/tmp/a.txt","/tmp/aln.txt")
Si le fichier pPathNouveauLien , existe déjà, retournez -2. Si le fichier pPathExistant est inexistant,
retournez -1. Si tout se passe bien, retournez 0. */
int bd_hardlink(const char *pPathExistant, const char *pPathNouveauLien) {
	// TODO: TEST
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
	updateINodeStats(&IE_existant);
	updateINodeStats(&IE_dirNouveauLien);

	return 0; // En cas de succès
}

/* Cette fonction sert à retirer un fichier normal ( G_IFREG 5 à 1) du répertoire dans lequel il est contenu. Le
retrait se fait en décrémentant de 1 le nombre de lien ( st_nlink ) dans l’i-node du fichier pFilename
et en détruisant l’entrée dans le fichier répertoire dans lequel pFilename se situe. Si st_nlink tombe
à zéro, vous devrez libérer cet i-node et ses blocs de données associés. Si après bd_unlink le nombre
de lien n’est pas zéro, vous ne pouvez pas libérer l’ i-node, puisqu’il est utilisé ailleurs (via un hardlink).
N’oubliez-pas de compacter les entrées dans le tableau de DirEntry du répertoire, si le fichier détruit
n’est pas à la fin de ce tableau. Si pFilename n’existe pas retournez -1. S’il n’est pas un fichier régulier
G_IFREG , retournez -2. Autrement, retourner 0 pour indiquer le succès.
*Un lien symbolique est aussi un fichier régulier. Il faudra donc le retirer du répertoire.* */
int bd_unlink(const char *pFilename) {
	// TODO à compléter
	return -1; // Si pFilename n'existe pas
	return -2; // Le fichier n'est pas un fichier régulier G_IFREG
	return 0; // En cas de succès
}

/* Cette fonction change la taille d’un fichier présent sur le disque. Pour les erreurs, la fonction retourne -1
si le fichier pFilename est inexistant, -2 si pFilename est un répertoire. Autrement, la fonction retourne
la nouvelle taille du fichier. Si NewSize est plus grand que la taille actuelle, ne faites rien et retournez la
taille actuelle comme valeur. N’oubliez-pas de marquer comme libre les blocs libérés par cette
fonction, si le changement de taille est tel que certains blocs sont devenus inutiles. Dans notre cas,
ce sera si on tronque à la taille 0. */
int bd_truncate(const char *pFilename, int NewSize) {
	// TODO à compléter
	return -1; // Le fichier pFilename est inexistant
	return -2; // Si pFilename est un répertoire
	// return la nouvelle taille du fichier;
}

/* Cette fonction sert à effacer un répertoire vide, i.e. s’il ne contient pas d’autre chose que les deux
répertoires « . » et « .. ». Si le répertoire n’est pas vide, ne faites rien et retournez -3. N’oubliez-pas de
décrémenter st_nlink pour le répertoire parent « .. ». Si le répertoire est inexistant, retourner -1. Si
c’est un fichier régulier, retournez -2. Autrement, retournez 0 pour indiquer le succès. */
int bd_rmdir(const char *pFilename) {
	// TODO à compléter
	return -1; // Si le répertoire pFilename st inexistant
	return -2; // si pFilename est un fichier régulier
	return -3; // Si le répertore n'est pas vide
	return 0; // En cas de succès
}

/* Cette fonction sert à déplacer ou renommer un fichier ou répertoire pFilename. Le nom et le répertoire
destination est le nom complet pFilenameDest. Par exemple, bd_rename("/tmp/a.txt","/doc/c.txt") va déplacer le f
ichier de /tmp vers /doc , et aussi renommer le fichier de a.txt à c.txt . N’oubliez-pas de retirer le fichier
(ou répertoire) du répertoire initial. Aussi, faites attention à ne pas faire d’erreur si vous manipuler le compteur
de lien. Si le fichier pFilename est un répertoire, n’oubliez-pas de mettre à jour le numéro d’i-node du répertoire
parent « .. ». En cas de succès, retournez 0. Attention! Il se peut que le répertoire soit le même pour
pFilename et pFilenameDest . Votre programme doit pouvoir supporter cela, comme dans le cas
bd_hardlink . Si le fichier pFilename est inexistant, ou si le répertoire de pFilenameDest est
inexistant, retournez -1. Pour vous simplifier la vie, vous n’avez pas besoin de gérer le cas invalide où le
fichier déplacé pFilename est un répertoire, et la destination pFilenameDest est un sous-répertoire
de celui-ci. */
int bd_rename(const char *pFilename, const char *pDestFilename) {
	// TODO à compléter
	return -1; // Le fichier pFilename est inexistant
	return -1; // Le répertoire pDestFilename est inexistant
	return 0; // En cas de succès
}

/* Cette fonction est utilisée pour permettre la lecture du contenu du répertoire pDirLocation . Si le
répertoire pDirLocation est valide, il faut allouer un tableau de DirEntry (via malloc ) de la bonne
taille et recopier le contenu du fichier répertoire dans ce tableau. Ce tableau est retourné à l’appelant via
le double pointeur ppListeFichiers . Par exemple, vous pouvez faire :
(*ppListeFichiers) = (DirEntry*)malloc(taille_de_données);
et traitez (*ppListeFichiers) comme un pointeur sur un tableau de DirEntry . La fonction
bd_readdir retourne comme valeur le nombre de fichiers et sous-répertoires contenus dans ce
répertoire pDirLocation (incluant . et ..). S’il y a une erreur, retourner -1. L’appelant sera en charge
de désallouer la mémoire via free . */
int bd_readdir(const char *pDirLocation, DirEntry **ppListeFichiers) {
	// TODO: test
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

/* Cette fonction est utilisée pour créer un lien symbolique vers pPathExistant . Vous devez ainsi créer
un nouveau fichier pPathNouveauLien , en prenant soin que les drapeaux G_IFLNK et G_IFREG soient
tous les deux à 1. La chaîne de caractère pPathExistant est simplement copiée intégralement dans le
nouveau fichier créé (pensez à réutiliser bd_write ici). Ne pas vérifier la validité de pPathExistant .
Assurez-vous que le répertoire qui va contenir le lien spécifié dans pPathNouveauLien existe, sinon
retournez -1. Si le fichier pPathNouveauLien , existe déjà, retournez -2. Si tout se passe bien,
retournez 0. ATTENTION! Afin de vous simplifier la vie, si une commande est envoyée à votre système
UFS sur un lien symbolique, ignorez ce fait. Ainsi, si / slnb.txt pointe vers /b.txt et que vous recevez
la commande ./ufs read /slnb.txt 0 40 , cette lecture retournera /b.txt et non pas le contenu
de b.txt . La commande suivante bd_readlink sera utilisée par le système d’exploitation pour faire le
déréférencement du lien symbolique, plus tard quand nous allons le monter dans Linux avec FUSE. */
int bd_symlink(const char *pPathExistant, const char *pPathNouveauLien) {
	// TODO à compléter
	return -1; // Le répertoire qui va contenir le lien spécifié par pPathNouveauLien est inexistant
	return -2; // Si le fichier pPathNouveauLien existe déjà
	return 0; // En cas de succès
}

/* Cette fonction est utilisée pour copier le contenu d’un lien symbolique pPathLien , dans le buffer
pBuffer de taille sizeBuffer . Ce contenu est une chaîne de caractère représentant le path du fichier
sur lequel ce lien symbolique pointe. Cette fonction permettra ainsi au système de fichier, une fois
montée dans Linux, de déréférencer les liens symboliques. Si le fichier pPathLien n’existe pas ou qu’il
n’est pas un lien symbolique, retournez -1. Sinon, retournez le nombre de caractères lus. */
int bd_readlink(const char *pPathLien, char *pBuffer, int sizeBuffer) {
	// TODO à compléter
    return -1; // Si le fichier pPathLien est inexistant
    return -1; // Si le fichier pPathLien n'est pas un lien symbolique
	// return le nombre de caractères lus;
}
