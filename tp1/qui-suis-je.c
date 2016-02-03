#include <unistd.h>
#include <stdio.h>

int main() {
    printf(" Bonjour! Le numero de l'utilisateur est %d\n", getuid());
    printf(" numero du processus est %d\n", getpid());
    printf(" - nom Alexandre Picard-Lemieux\n");
    printf(" - nom Gael Dostie\n");
    return 0;
}
