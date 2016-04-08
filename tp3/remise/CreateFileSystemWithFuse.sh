#!/bin/bash


echo "Voici le script pour tester notre système de fichier avec FUSE"

echo
echo "--------------------------------------------------------------------"
echo "                     Vider les exécutables"
echo "--------------------------------------------------------------------"
make clean

echo
echo "--------------------------------------------------------------------"
echo "                     Compiler les exécutables"
echo "--------------------------------------------------------------------"
make all

echo
echo "--------------------------------------------------------------------"
echo "                     Suppression du dossier temporaire"
echo "--------------------------------------------------------------------"
rm -rf /tmp/glo

echo
echo "--------------------------------------------------------------------"
echo "                     Création du dossier temporaire"
echo "--------------------------------------------------------------------"
mkdir /tmp/glo

echo
echo "--------------------------------------------------------------------"
echo "                     Partir le système de fichier"
echo "--------------------------------------------------------------------"
echo
echo "Faire Ctrl+C pour arrêter le système de fichier."
./glofs /tmp/glo -f -ouse_ino
