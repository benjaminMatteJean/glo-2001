all: ChercherSHA256

ChercherSHA256: sha256.o ChercherSHA256.o 
	gcc sha256.o ChercherSHA256.o -o ChercherSHA256 -lrt -lpthread

sha256.o: sha256.c
	gcc -c -g sha256.c

ChercherSHA256.o: ChercherSHA256.c
	gcc -c -g ChercherSHA256.c

clean: 
	rm -f sha256.o ChercherSHA256.o ChercherSHA256 *.*~
