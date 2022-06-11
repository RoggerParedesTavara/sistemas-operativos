#ifndef NARGS
#define NARGS 4
#endif
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

void ejecutar(char *args[]);
void limpiar(char *args[], int i);

void
ejecutar(char *args[])
{
	int i = fork();
	if (i < 0) {
		perror("Error en fork");
		exit(-1);
	}

	if (i == 0) {
		execvp(args[0], args);
	} else {
		wait(NULL);
	}
}

void
limpiar(char *args[], int i)
{
	for (int j = 1; j < i; j++) {
		free(args[j]);
	}
}

int
main(int argc, char *argv[])
{
	(void) argc;  // evito el warning: unused parameter 'argc'
	size_t len = 0;
	size_t read = 0;
	char *line = NULL;
	char *args[NARGS + 2] = { argv[1] };
	int i = 1;
	while ((read = getline(&line, &len, stdin) != -1)) {
		if (i == (NARGS + 1)) {  // ya lei NARGS veces
			args[i] = NULL;
			ejecutar(args);
			limpiar(args, i);
			i = 1;
		}
		line[strlen(line) - 1] = '\0';  // borro el \n del final
		args[i] = strdup(line);
		i++;
	}

	if (i > 1) {  // Quedaron elementos sin evaluar
		args[i] = NULL;
		ejecutar(args);
		limpiar(args, i);
	}

	free(line);
	exit(EXIT_SUCCESS);
}