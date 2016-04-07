#!/bin/bash


echo "Voici le script pour tester notre système de fichier avec FUSE"


echo "----- Vider les exécutables -----"
make clean

echo "----- Compiler les exécutables -----"
make all

echo "----- Suppression du dossier temporaire -----"
rm -rf /tmp/glo

echo "--- Création du dossier temporaire -----"
mkdir /tmp/glo

echo "--- Partir le système de fichier -----"
./glos /tmp/glo -f -ouse_ino
