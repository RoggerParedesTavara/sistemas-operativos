#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {

    int n = atoi(argv[1]);

    int numeros[n];
    for(int i = 0; i < n, i++) {
        numeros[i] = i;
    }

    int p = pipe(fds);
    if (p < 0) {
        perror("Error en pipe");
        exit(-1);
    }

    int i = fork();
	if (i < 0) {
        perror("Error en fork");
        exit(-1);
    }


    if (i == 0) {
        printf("[hijo] mi pid es: %d\n", getpid());
        close(fds[1]);
        int i2 = fork();
        if (i2 < 0) {
            perror("Error en fork");
            exit(-1);
        }

        
    } else {
        printf("[padre] mi pid es: %d\n", getpid());
    }
}