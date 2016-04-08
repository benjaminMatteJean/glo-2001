#!/bin/bash

echo
echo "--------------------------------------------------------------------"
echo "                     Deplacement vers le répertoire temporaire"
echo "--------------------------------------------------------------------"
echo "cd /tmp/glo"
cd /tmp/glo


echo
echo "--------------------------------------------------------------------"
echo "                     Test de la commande ls"
echo "--------------------------------------------------------------------"
echo "ls"
ls

echo
echo "--------------------------------------------------------------------"
echo "                     Test de la commande ls -i"
echo "--------------------------------------------------------------------"
echo "ls -i"
ls -i

echo
echo "--------------------------------------------------------------------"
echo "                     Test de la commande mkdir"
echo "--------------------------------------------------------------------"
echo
echo "Création du répertoire test"
echo "mkdir test"
mkdir test
echo "ls -l"
ls -l

echo
echo "--------------------------------------------------------------------"
echo "                     Test de la commande mv"
echo "--------------------------------------------------------------------"
echo
echo "Déplacer le fichier b.txt dans le répertoire test"
echo "mv b.txt test"
mv b.txt test
echo "ls -l test"
ls  -l test

echo
echo "--------------------------------------------------------------------"
echo "                     Test de la commande touch"
echo "--------------------------------------------------------------------"
echo "touch test/b.txt"
touch test/b.txt


echo
echo "--------------------------------------------------------------------"
echo "                     Test de la commande ln"
echo "--------------------------------------------------------------------"
echo
echo "Création d'un lien vers Bonjour/LesAmis.txt"
echo "ln Bonjour/LesAmis.txt"
ln Bonjour/LesAmis.txt
echo "ls -la"
ls -la

echo
echo "--------------------------------------------------------------------"
echo "                     Test de la commande rmdir"
echo "--------------------------------------------------------------------"
echo
echo "Test de suppresion du répertoire test, mais il devrait"
echo " avoir une erreur"
echo "rmdir test"
rmdir test
echo
echo "Création d'un répertoire test2"
echo "mkdir test2"
mkdir test2
echo "ls -l"
ls -l
echo "Suppresion du répertoire test2"
echo "rmdir test2"
rmdir test2
echo "ls -l"
ls -l

echo
echo "--------------------------------------------------------------------"
echo "                     Test de la commande cd"
echo "--------------------------------------------------------------------"
echo "ls -l"
ls -l
echo "cd doc"
cd doc
echo "ls -l"
ls -l
echo "cd .."
cd ..

echo
echo "--------------------------------------------------------------------"
echo "                     Test de la commande cp"
echo "--------------------------------------------------------------------"
echo
echo "Copier du fichier Bonjour/LesAmis.txt vers test/"
echo "cp Bonjour/LesAmis.txt test/"
cp Bonjour/LesAmis.txt test/
echo "ls -l test/"
ls test/

echo
echo "--------------------------------------------------------------------"
echo "                     Test de la commande ln -s"
echo "--------------------------------------------------------------------"
echo
echo "Création d'un lien symbolique du fichier test/b.txt dans"
echo " le répertoire root avec le nom symlinkb.txt"
echo "ln -s test/b.txt symlinkb.txt"
ln -s test/b.txt symlinkb.txt
echo "ls -la"
ls -la

echo
echo "--------------------------------------------------------------------"
echo "                     Test de la commande more"
echo "--------------------------------------------------------------------"
echo "more symlinkb.txt"
more symlinkb.txt

echo
echo "--------------------------------------------------------------------"
echo "                     Test de la commande ls -la"
echo "--------------------------------------------------------------------"
echo "ls -la"
ls -la

echo
echo "--------------------------------------------------------------------"
echo "                     Test de la commande gedit"
echo "--------------------------------------------------------------------"
echo "gedit symlinkb.txt"
gedit symlinkb.txt
echo "Une fenêtre de gedit devrait ouvrir"
