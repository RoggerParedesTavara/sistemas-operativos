#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
        int fds_h[2];
	int fds_p[2];
        int msg= random();
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
                printf("[hijo] recibi: %d\n", recv_h);
                close(fds_p[0]);
		
                write(fds_h[1], &msg, sizeof(msg));
                close(fds_h[1]);

        } else {
                printf("[padre] mi pid es: %d\n", getpid());
        	// El padre va a escribir en fds_p y leer en fds_h
                close(fds_h[1]);
		close(fds_p[0]);
        	
                write(fds_p[1], &msg, sizeof(msg));
                close(fds_p[1]);

		int recv_p = 0;
		read(fds_h[0], &recv_p, sizeof(recv_p));
		printf("[padre] recibi: %d\n", recv_p);
                close(fds_h[0]);

                int ret = wait(NULL);
                printf("Proceso %d termino \n", ret);
       }
}
