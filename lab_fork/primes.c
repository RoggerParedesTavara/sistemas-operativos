#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

void fork_error(int num);
void pipe_error(int num);
void auxiliar_primos(int fd);
void write_error(int num);

void
fork_error(int num)
{
	if (num < 0) {
		perror("Error en fork");
		exit(-1);
	}
}

void
pipe_error(int num)
{
	if (num < 0) {
		perror("Error en pipe");
		exit(-1);
	}
}

void
write_error(int num)
{
	if (num < 0) {
		perror("Error en write");
		exit(-1);
	}
}

void
auxiliar_primos(int fd)
{  // Recibe el numero del fd por donde el padre le manda mensajes
	int primo;

	if (read(fd,
	         &primo,
	         sizeof(primo))) {  // Si el pipe izq no me mando nada --> devuelve 0

		printf("primo %d\n", primo);  // Asumo que es primo

		// Creo al "hermano" derecho y la tuberia que los va a comunicar
		int fd2[2];
		int p2 = pipe(fd2);
		pipe_error(p2);
		int i2 = fork();
		fork_error(i2);

		if (i2 == 0) {      // hijo derecho
			close(fd);  // cierro el pipe que comunica con el "abuelo"
			close(fd2[1]);  // al pipe izquierdo no le voy a mandar nada
			auxiliar_primos(fd2[0]);
			close(fd2[0]);
		} else {
			close(fd2[0]);
			int aux;
			while (read(fd,
			            &aux,
			            sizeof(aux))) {  // Mientras el pipe izquierdo me siga mandando numeros
				// Chequeo si es multiplo del primo actual, si no es asi se lo mando al pipe derecho
				if (aux % primo != 0) {
					int w = write(fd2[1], &aux, sizeof(aux));
					write_error(w);
				}
			}
			close(fd2[1]);
			close(fd);
			wait(NULL);
		}
	}
	close(fd);  // Si no hay nada mas para leer, nunca entra al if de arriba y nunca se cierra este fds
	exit(-1);
}


int
main(int argc, char *argv[])
{
	if (argc != 2) {
		perror("Error con cantidad de argumentos");
		exit(-1);
	}

	int fds[2];
	int p = pipe(fds);
	pipe_error(p);

	int i = fork();
	fork_error(i);

	if (i == 0) {
		close(fds[1]);  // No le va a mandar nada al padre
		auxiliar_primos(fds[0]);
	} else {
		close(fds[0]);  // El primer proceso nunca va a leer

		// Primer proceso manda los numeros de 2 a n
		for (int i = 2; i <= atoi(argv[1]); i++) {
			int w = write(fds[1], &i, sizeof(i));
			write_error(w);
		}
		close(fds[1]);
		wait(NULL);
	}
	exit(EXIT_SUCCESS);
}
