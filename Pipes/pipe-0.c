#include <stdlib.h>
#include <stdio.h>

int main(int argc, char* argv[]) {
	int fds[2];
	int msg = 42;
	int r = pipe(fds);
	
	if (r < 0) {
		perror("Error en pipe");
		exit(-1);
	}
	
	printf("Lectura: %d, Escritura: %d\n", fds[0], fds[1]);
	// read(fds[0], %msg, sizeof(msg)); // ????
	// Escribo en el pipe
	write(fds[1], &msg, sizeof(msg));
	
	int recibido = 0;
	read(fds[0], &recibido, sizeof(recibido));
	printf("Recibi: %d\n", recibido);
	
	close(fds[0]);
	close(fds[1]);
}
