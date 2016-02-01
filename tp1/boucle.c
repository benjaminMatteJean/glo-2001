#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv) {
  long int i=0;
  while (1)
  {
     printf("Boucle ");
     usleep(10000); // Faire dormir pendant 1/10 de seconde
  }
  return 0;
} 
