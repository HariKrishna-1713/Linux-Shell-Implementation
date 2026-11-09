#ifndef MINISHELL_H
#define MINISHELL_H

#include<signal.h>

#define MAX_INPUT 100
#define MAX_ARGS 20

// Function declarations for command parsing and execution.
void parse_command(char *cmd, char *argv[]);
int custom_command(char *argv[]);
int execute_command(char *argv[]);
int handle_redirection(char *argv[]);
int execute_pipeline(char *argv[]);
int execute_background(char *argv[]);
void repeated_background_jobs(void);

// Extern declarations for signal flags defined in signal.c.
extern volatile sig_atomic_t sigint_flag;
extern volatile sig_atomic_t foreground_running;
extern volatile sig_atomic_t child_signal;

// Signal handler and initialization function declarations.
void setup_signals(void);
void myhandler_sigint(int sig);
void myhandler_sigchld(int sig);

#endif
