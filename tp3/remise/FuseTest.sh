#!/bin/bash

echo
echo "--------------------------------------------------------------------"
echo "                     Deplacement vers le répertoire temporaire"
echo "--------------------------------------------------------------------"
cd /tmp/glo

echo
echo "--------------------------------------------------------------------"
echo "                     Test de la commande ls"
echo "--------------------------------------------------------------------"
ls

echo
echo "--------------------------------------------------------------------"
echo "                     Test de la commande ls -i"
echo "--------------------------------------------------------------------"
ls -i

echo
echo "--------------------------------------------------------------------"
echo "                     Test de la commande mkdir"
echo "--------------------------------------------------------------------"
echo
echo "Création du répertoire test"
mkdir test
ls

echo
echo "--------------------------------------------------------------------"
echo "                     Test de la commande mv"
echo "--------------------------------------------------------------------"
echo
echo "Déplacer le fichier b.txt dans le répertoire test"
mv b.txt test
ls test

echo
echo "--------------------------------------------------------------------"
echo "                     Test de la commande touch"
echo "--------------------------------------------------------------------"
touch test/b.txt

echo
echo "--------------------------------------------------------------------"
echo "                     Test de la commande ln"
echo "--------------------------------------------------------------------"
echo
echo "Création d'un lien vers Bonjour/LesAmis.txt"
ln Bonjour/LesAmis.txt
ls -la

echo
echo "--------------------------------------------------------------------"
echo "                     Test de la commande rmdir"
echo "--------------------------------------------------------------------"
echo
echo "Test de suppresion du répertoire test, mais il devrait"
echo " avoir une erreur"
rmdir test
echo
echo "Création d'un répertoire test2"
mkdir test2
ls
echo "Suppresion du répertoire test2"
rmdir test2
ls

echo
echo "--------------------------------------------------------------------"
echo "                     Test de la commande cd"
echo "--------------------------------------------------------------------"
ls -l
cd doc
ls -l
cd ..

echo
echo "--------------------------------------------------------------------"
echo "                     Test de la commande cp"
echo "--------------------------------------------------------------------"
echo
echo "Copier du fichier Bonjour/LesAmis.txt vers test/"
cp Bonjour/LesAmis.txt test/
ls test/

echo
echo "--------------------------------------------------------------------"
echo "                     Test de la commande more"
echo "--------------------------------------------------------------------"
more symlinkb.txt

echo
echo "--------------------------------------------------------------------"
echo "                     Test de la commande ln -s"
echo "--------------------------------------------------------------------"
echo
echo "Création d'un lien symbolique du fichier test/b.txt dans"
echo " le répertoire root avec le nom symlinkb.txt"
ln -s test/b.txt symlinkb.txt
ls -la

echo
echo "--------------------------------------------------------------------"
echo "                     Test de la commande ls -la"
echo "--------------------------------------------------------------------"
ls -la

echo
echo "--------------------------------------------------------------------"
echo "                     Test de la commande gedit"
echo "--------------------------------------------------------------------"
gedit symlinkb.txt
echo "Une fenêtre de gedit devrait ouvrir"
