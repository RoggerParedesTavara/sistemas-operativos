#include "exec.h"

// sets "key" with the key part of "arg"
// and null-terminates it
//
// Example:
//  - KEY=value
//  arg = ['K', 'E', 'Y', '=', 'v', 'a', 'l', 'u', 'e', '\0']
//  key = "KEY"
//
static void
get_environ_key(char *arg, char *key)
{
	int i;
	for (i = 0; arg[i] != '='; i++)
		key[i] = arg[i];

	key[i] = END_STRING;
}

// sets "value" with the value part of "arg"
// and null-terminates it
// "idx" should be the index in "arg" where "=" char
// resides
//
// Example:
//  - KEY=value
//  arg = ['K', 'E', 'Y', '=', 'v', 'a', 'l', 'u', 'e', '\0']
//  value = "value"
//
static void
get_environ_value(char *arg, char *value, int idx)
{
	size_t i, j;
	for (i = (idx + 1), j = 0; i < strlen(arg); i++, j++)
		value[j] = arg[i];

	value[j] = END_STRING;
}

// sets the environment variables received
// in the command line
//
// Hints:
// - use 'block_contains()' to
// 	get the index where the '=' is
// - 'get_environ_*()' can be useful here
static void
set_environ_vars(char **eargv, int eargc)
{
	for (int i = 0; i < eargc; i++) {
		int idx = block_contains(eargv[i], '=');
		if (idx > 0) {
			char key[BUFLEN], value[BUFLEN];
			get_environ_key(eargv[i], key);
			get_environ_value(eargv[i], value, idx);

			if (setenv(key, value, 1) != 0) {
				perror("Error en setenv");
			}
		}
	}
}

// opens the file in which the stdin/stdout/stderr
// flow will be redirected, and returns
// the file descriptor
//
// Find out what permissions it needs.
// Does it have to be closed after the execve(2) call?
//
// Hints:
// - if O_CREAT is used, add S_IWUSR and S_IRUSR
// 	to make it a readable normal file
static int
open_redir_fd(char *file, int flags)
{
	if (flags & O_CREAT) {
		return open(file, flags, S_IWUSR | S_IRUSR);
	}
	return open(file, flags);
}

// Si hay error en dup2 cierra el fd y hace un exit(-1)
void
error_dup(int dup2, int fd)
{
	if (dup2 < 0) {
		close(fd);
		perror("Error en dup2");
		exit(EXIT_FAILURE);
	}
}

// Depende el fd que hay que redireccionar hace las llamadas correspondientes
static void
redir_fd(struct execcmd *r)
{
	if (strlen(r->out_file) > 0) {  // OUT
		int out_fd =
		        open_redir_fd(r->out_file,
		                      O_CREAT | O_TRUNC | O_RDWR | O_CLOEXEC);

		error_dup(dup2(out_fd, OUT), out_fd);
		close(out_fd);
	}
	if (strlen(r->in_file) > 0) {  // IN
		int in_fd = open_redir_fd(r->in_file, O_CLOEXEC | O_RDONLY);

		error_dup(dup2(in_fd, IN), in_fd);
		close(in_fd);
	}
	if (strlen(r->err_file) > 0) {                 // ERR
		if (strcmp(r->err_file, "&1") == 0) {  // caso 2>&1
			if (dup2(OUT, ERR) < 0) {
				perror("Error en dup2");
				exit(EXIT_FAILURE);
			}
		} else {
			int err_fd = open_redir_fd(r->err_file,
			                           O_CREAT | O_RDWR | O_CLOEXEC);
			error_dup(dup2(err_fd, ERR), err_fd);
			close(err_fd);
		}
	}
}

void
coordinator_pipe(struct pipecmd *cmd)
{
	int fds[2], f1, f2;
	int p = pipe(fds);
	error(p);
	f1 = fork();
	error(f1);
	if (f1 == 0) {  // hijo izquierdo
		close(fds[READ]);
		error_dup(dup2(fds[WRITE], WRITE), fds[WRITE]);
		close(fds[WRITE]);
		exec_cmd(cmd->leftcmd);
	} else {
		close(fds[WRITE]);
		f2 = fork();
		error(f2);
		if (f2 == 0) {  // hijo derecho se ejecuta
			error_dup(dup2(fds[READ], READ), fds[READ]);
			close(fds[READ]);
			exec_cmd(cmd->rightcmd);
			_exit(-1);
		} else {
			close(fds[READ]);
			wait(NULL);
			wait(NULL);
		}
	}
}

void
aux_back(struct backcmd *b)
{
	int f1 = fork();
	error(f1);
	if (f1 == 0) {
		exec_cmd(b->c);
	}
	int wstatus;
	waitpid(f1, &wstatus, WNOHANG);
	printf_debug("[PID=%d]\n", f1);
	_exit(EXIT_SUCCESS);
}

// executes a command - does not return
//
// Hint:
// - check how the 'cmd' structs are defined
// 	in types.h
// - casting could be a good option
void
exec_cmd(struct cmd *cmd)
{
	// To be used in the different cases
	struct execcmd *e;
	struct backcmd *b;
	struct execcmd *r;
	struct pipecmd *p;

	switch (cmd->type) {
	case EXEC:
		// spawns a command
		e = (struct execcmd *) cmd;
		set_environ_vars(e->eargv, e->eargc);
		error(execvp(e->argv[0], e->argv));
		break;

	case BACK: {
		// runs a command in background
		b = (struct backcmd *) cmd;
		aux_back(b);
		break;
	}

	case REDIR: {
		// changes the input/output/stderr flow
		r = (struct execcmd *) cmd;
		redir_fd(r);
		r->type = EXEC;
		exec_cmd((struct cmd *) r);
		break;
	}

	case PIPE: {
		// pipes two commands
		p = (struct pipecmd *) cmd;
		coordinator_pipe(p);

		// free the memory allocated
		// for the pipe tree structure
		free_command(parsed_pipe);
		_exit(EXIT_SUCCESS);
		break;
	}
	}
}
