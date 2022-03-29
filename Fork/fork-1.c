#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>


int main(int argc, char* argv[]) {
	int a = 4;
	int i = fork();
	a = 5;
	if (i < 0) {
		printf("Error en fork! %d\n", i);
		exit(-1);
	}

	if (i == 0) {
		printf("[hijo] mi pid es: %d\n", getpid());
		printf("[hijo] a=%d\n", a);
	} else {
		a = 6;
		printf("[padre] mi pid es: %d\n", getpid());
		printf("[padre] a=%d\n", a);
	}

	printf("Terminando\n");
	exit(0);
}

