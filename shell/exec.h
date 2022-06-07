#ifndef EXEC_H
#define EXEC_H

#include "defs.h"
#include "types.h"
#include "utils.h"
#include "freecmd.h"
#include "printstatus.h"

extern struct cmd *parsed_pipe;
extern int status;

void exec_cmd(struct cmd *c);
void error_dup(int dup2, int fd);
void coordinator_pipe(struct pipecmd *cmd);
void aux_back(struct backcmd *b);

#endif  // EXEC_H
