#include "builtin.h"
#include "utils.h"

// returns true if the 'exit' call
// should be performed
//
// (It must not be called from here)
int
exit_shell(char *cmd)
{
	return (strcmp(cmd, "exit") == 0);
}

// returns true if "chdir" was performed
//  this means that if 'cmd' contains:
// 	1. $ cd directory (change to 'directory')
// 	2. $ cd (change to $HOME)
//  it has to be executed and then return true
//
//  Remember to update the 'prompt' with the
//  	new directory.
//
// Examples:
//  1. cmd = ['c','d', ' ', '/', 'b', 'i', 'n', '\0']
//  2. cmd = ['c','d', '\0']
int
cd(char *cmd)
{
	char *directory;
	if (strcmp(cmd, "cd\0") == 0) {  // ejemplo 2
		directory = getenv("HOME");
	} else if (strncmp(cmd, "cd ", 3) == 0) {  // ejemplo 1
		directory = cmd + 3;
	} else {
		return EXIT_SUCCESS;
	}

	// Cambio de directorio y actualizo el prompt
	if (chdir(directory) < 0) {
		char buf[BUFLEN];
		snprintf(buf, sizeof buf, "cannot cd to %s ", directory);
		status = 1;
		perror(buf);
	} else {
		snprintf(promt, sizeof promt, "(%s)", getcwd(directory, PRMTLEN));
	}

	return 1;
}

// returns true if 'pwd' was invoked
// in the command line
//
// (It has to be executed here and then
// 	return true)
int
pwd(char *cmd)
{
	if (strcmp(cmd, "pwd") == 0) {
		char buf[PRMTLEN];
		printf("%s\n", getcwd(buf, PRMTLEN));
		status = 1;
		return 1;
	}
	return EXIT_SUCCESS;
}
