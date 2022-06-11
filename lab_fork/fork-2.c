#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
	char* msg = "fisop\n";
	// Abro un archivo y si no existe lo creo
	int fd = open("hola.txt", O_CREAT | O_RDWR, 0644);
	int i = fork();

	if (i < 0) {
		printf("Error en fork! %d\n", i);
		exit(-1);
	}

	if (i == 0) {
		printf("[hijo] mi pid es: %d\n", getpid());
		write(fd, msg, 6);
		close(fd);
	} else {
		printf("[padre] mi pid es: %d\n", getpid());
	}
	
	printf("Terminando\n");
	exit(0);
}
