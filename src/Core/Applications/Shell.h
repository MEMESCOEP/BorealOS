#ifndef SHELL_H
#define SHELL_H

/* DEFINITIONS */
#define MAX_CMD_LEN 512
#define MAX_ARGS 128


/* VARIABLES */
extern char Command[];


/* FUNCTIONS */
size_t SplitCommand(char *FullCommand, char *ArgListOut[]);
void StartShell();

#endif