#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
	printf("Mi PID es: %d\n", getpid());
	
	for (int i = 0; i < 12; i++) {
		int r = fork();
		if (r < 0) {
			perror("Error en fork");
			exit(-1);
		}
		printf("[%d] Hola!\n", getpid());
	}
	
	printf("Terminando\n");
	exit(0);
}
