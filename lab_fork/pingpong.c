#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>

static void
aux_error(int x)
{
	if (x < 0) {
		exit(-1);
	}
}

int
main()
{
	int fds_h[2], fds_p[2];
	int p1 = pipe(fds_p);
	int p2 = pipe(fds_h);
	srand(time(NULL));

	if (p1 < 0 || p2 < 0) {
		perror("Error en pipe");
		exit(-1);
	}

	int i = fork();
	int msg = random();

	if (i < 0) {
		perror("Error en fork");
		exit(-1);
	}

	if (i == 0) {
		// El hijo va a escribir en fds_h y leer en fds_p
		close(fds_p[1]);
		close(fds_h[0]);

		printf("\nDonde fork me devuelve 0:\n");
		printf("  - getpid me devuelve: %d\n", getpid());
		printf("  - getppid me devuelve: %d\n", getppid());

		int rec_h = 0;
		int r2 = read(fds_p[0], &rec_h, sizeof(rec_h));
		aux_error(r2);
		printf("  - recibo valor %d vía fd=%d\n", rec_h, fds_p[0]);
		close(fds_p[0]);  // No voy a recibir mas nada

		printf("  - reenvío valor en fd=%d y termino\n", fds_h[1]);
		int w2 = write(fds_h[1], &msg, sizeof(msg));
		aux_error(w2);
		close(fds_h[1]);

	} else {
		printf("Hola, soy PID %d:\n", getpid());
		printf("  - primer pipe me devuelve: [%d, %d]\n",
		       fds_p[0],
		       fds_p[1]);
		printf("  - segundo pipe me devuelve: [%d, %d]\n",
		       fds_h[0],
		       fds_h[1]);

		// El padre va a escribir en fds_p y leer en fds_h
		close(fds_h[1]);
		close(fds_p[0]);

		printf("\nDonde fork me devuelve %d:\n", i);
		printf("  - getpid me devuelve: %d\n", getpid());
		printf("  - getppid me devuelve: %d\n", getppid());
		printf("  - random me devuelve: %d\n", msg);
		printf("  - envío valor %d a través de fd=%d\n", msg, fds_p[1]);

		int w1 = write(fds_p[1], &msg, sizeof(msg));
		aux_error(w1);
		close(fds_p[1]);  // No va a mandar mas nada

		int rec_p = 0;
		int r1 = read(fds_h[0], &rec_p, sizeof(rec_p));
		aux_error(r1);
		printf("\nHola, de nuevo PID %d:\n", getpid());
		printf("  - recibí valor %d vía fd=%d\n", rec_p, fds_h[0]);
		close(fds_h[0]);  // No voy a recibir mas nada

		wait(NULL);
	}
	exit(EXIT_SUCCESS);
}