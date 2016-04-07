#!/bin/bash

echo "Deplacement vers le répertoire temporaire"
cd /tmp/glo

echo "----- Test de la commande ls -----"
ls
echo "----- Test de la commande ls -la -----"
ls –la
echo "----- Test de la commande ls -i -----"
ls -i
echo "----- Test de la commande mkdir -----"
mkdir test
ls
echo "----- Test de la commande mv -----"
mv b.txt test
ls test
echo "----- Test de la commande touch -----"
touch test/b.txt
echo "----- Test de la commande ln -----"
ln Bonjour/LesAmis.txt
ls
echo "----- Test de la commande rmdir -----"
rmdir test
mkdir test2
ls
rmdir test2
ls
echo "----- Test de la commande cd -----"
cd doc
ls
cd ..
echo "----- Test de la commande cp -----"
cp Bonjour/LesAmis.txt test/
ls test/
echo "----- Test de la commande more -----"
more symlinkb.txt
echo "----- Test de la commande ls -s -----"
ln -s test/b.txt symlinkb.txt
ls
echo "----- Test de la commande gedit -----"
gedit symlinkb.txt
