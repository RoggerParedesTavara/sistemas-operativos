#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
	printf("Mi PID es: %d\n", getpid());
	
	int i = fork();
	if (i < 0) {
		printf("Error en fork! %d\n", i);
		exit(-1);
	}

	if (i == 0) {
		printf("Soy el proceso hijo y mi pid es: %d\n", getpid());
	} else {
	printf("Soy el proceso padre y mi pid es: %d\n", getpid());
	}

	printf("Terminando\n");
	exit(0);
}
