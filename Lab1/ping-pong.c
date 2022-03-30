#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
        int fds_h[2];
	int fds_p[2];
        int msg_p = random();
	int msg_h = random();
        pipe(fds_p);
	pipe(fds_h);
        int i = fork();

	if (i < 0) {
                perror("Error en fork");
                exit(-1);
        }

        if (i == 0) {
                printf("[hijo] mi pid es: %d\n", getpid());
        	// El hijo va a escribir en fds_h y leer en fds_p
		close(fds_p[1]);
                close(fds_h[0]);
                int recv_h = 0;
                read(fds_p[0], &recv_h, sizeof(recv_h));
                printf("[hijo] lei: %d\n", recv_h);
                close(fds_p[0]);
		
		printf("[hijo] envio el mensaje: %d\n", msg_h);
                write(fds_h[1], &msg_h, sizeof(msg_h));
                close(fds_h[1]);

		printf("[hijo] termino\n");

        } else {
                printf("[padre] mi pid es: %d\n", getpid());
        	// El padre va a escribir en fds_p y leer en fds_h
                close(fds_h[1]);
		close(fds_p[0]);
        	
		printf("[padre] envio el mensaje: %d\n", msg_p);
                write(fds_p[1], &msg_p, sizeof(msg_p));
                close(fds_p[1]);

		int recv_p = 0;
		read(fds_h[0], &recv_p, sizeof(recv_p));
		printf("[padre] lei: %d\n", recv_p);
                close(fds_h[0]);

		printf("[padre] termino\n");
       }
}
