#include<stdio.h>
#include<signal.h>
#include<unistd.h>

#include "minishell.h"

volatile sig_atomic_t child_signal = 0;
volatile sig_atomic_t foreground_running = 0;
volatile sig_atomic_t sigint_flag = 0;

void myhandler_sigint(int sig)
{
        // If a foreground process is running, print a newline.
        // Otherwise, notify the shell to clear and redisplay the prompt.
        if(foreground_running)
        {
                write(STDOUT_FILENO, "\n", 1);
        }
        else
                sigint_flag = 1;
}

void myhandler_sigchld(int sig)
{
        // Notify the shell that a child process has changed its state.
        child_signal = 1;
}

void setup_signals(void)
{
        struct sigaction s;

        // Register the SIGINT handler (Ctrl+C).
        s.sa_handler = myhandler_sigint;
        sigemptyset(&s.sa_mask);
        s.sa_flags = 0;

        if(sigaction(SIGINT, &s, NULL) == -1)
        {
                perror("sigaction SIGINT");
        }

        // Register the SIGCHLD handler to detect child process state changes.
        s.sa_handler = myhandler_sigchld;
        sigemptyset(&s.sa_mask);
        s.sa_flags = SA_RESTART;

        if(sigaction(SIGCHLD, &s, NULL) == -1)
        {
                perror("sigaction SIGINT");
        }
}
