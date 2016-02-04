#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char* argv[]) {
    int pid;
    pid = fork();

    if(pid==0) {
        printf("Bonjour! Je suis le fils %d\n",getpid());
        sleep(120);
        exit(0);
    } else {
        printf("Coucou! Je suis le parent, et mon fils a le numéro de processus %d\n",pid);
        wait(0);
    }
}
