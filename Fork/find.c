#define _GNU_SOURCE
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#define FLAG "-i"

void buscar_en_directorio(DIR *dir,
                          char *a_buscar,
                          char path[],
                          char *(*ptr_fun)(const char *, const char *) );
void buscar_coincidencia(char *actual,
                         char *a_buscar,
                         char path[],
                         char *(*ptr_fun)(const char *, const char *) );
void es_directorio(DIR *actual,
                   char *nombre_dir,
                   char *a_buscar,
                   char path[],
                   char *(*ptr_fun)(const char *, const char *) );

void
buscar_coincidencia(char *actual,
                    char *a_buscar,
                    char path[],
                    char *(*ptr_fun)(const char *, const char *) )
{
	if ((*ptr_fun)(actual, a_buscar)) {
		printf("%s/%s\n", path, actual);
	}
}

void
es_directorio(DIR *actual,
              char *nombre_dir,
              char *a_buscar,
              char path[],
              char *(*ptr_fun)(const char *, const char *) )
{
	if ((strcmp(nombre_dir, ".") && strcmp(nombre_dir, "..")) != 0) {
		buscar_coincidencia(
		        nombre_dir,
		        a_buscar,
		        path,
		        ptr_fun);  // chequeo si el nombre del directorio coincide
		int fd = dirfd(actual);
		int nuevo_fd = openat(fd, nombre_dir, __O_DIRECTORY);
		if (nuevo_fd == -1) {
			perror("Error con openat");
			exit(-1);
		}

		DIR *nuevo_dir = fdopendir(nuevo_fd);
		char aux[PATH_MAX];
		strcpy(aux, path);
		strcat(aux, "/");
		strcat(aux, nombre_dir);
		buscar_en_directorio(nuevo_dir, a_buscar, aux, ptr_fun);
		close(nuevo_fd);
		closedir(nuevo_dir);
	}
}


void
buscar_en_directorio(DIR *dir,
                     char *a_buscar,
                     char path[],
                     char *(*ptr_fun)(const char *, const char *) )
{
	struct dirent *entry;
	while ((entry = readdir(dir))) {
		if ((entry->d_type == DT_DIR)) {
			es_directorio(dir, entry->d_name, a_buscar, path, ptr_fun);
		} else if (entry->d_type == DT_REG) {
			buscar_coincidencia(entry->d_name, a_buscar, path, ptr_fun);
		}
	}
}


int
main(int argc, char *argv[])
{
	if (argc < 2) {
		perror("Error con cantidad de argumentos");
		exit(-1);
	}

	DIR *directorio = opendir(".");
	if (directorio == NULL) {
		perror("Error con opendir");
		exit(-1);
	}

	char path[PATH_MAX] = ".";

	if (argc == 2) {  // case_sensitive
		buscar_en_directorio(directorio, argv[1], path, strstr);
	} else if (argc == 3 && strcmp(argv[1], FLAG) == 0) {
		buscar_en_directorio(directorio, argv[2], path, strcasestr);
	}
	closedir(directorio);
	exit(EXIT_SUCCESS);
}