#include<stdio.h>
#include<sys/wait.h>
#include<unistd.h>
#include<errno.h>
#include<readline/readline.h>
#include<stdlib.h>

#include "minishell.h"

int execute_background(char *argv[])
{
        int pid;

        pid = fork();

        if(pid == -1)
        {
                perror("fork");
                return -1;
        }

        if(!pid)
        {
                // Child creates its own process group to separate it from the shell.
                setpgid(0, 0);

                // Apply input/output redirection before executing the command.
                if(handle_redirection(argv) == -1)
                        exit(1);

                if((execvp(argv[0],argv))==-1)
                {
                        perror("execvp");
                        exit(1);
                }
        }
        else
        {
                // Parent also sets the child's process group to avoid race conditions.
                setpgid(pid, pid);

                printf("[Background PID: %d]\n", pid);
                return 0;
        }
}

void repeated_background_jobs(void)
{
        int ret, status;

        // Reap all completed child processes without blocking the shell.
        while(1)
        {
                ret = waitpid(-1, &status, WNOHANG);

                if(ret>0)
                {
                        // Notify the user and redisplay the shell prompt.
                        rl_crlf();
                        printf("\n[Background process %d completed]\n", ret);
                        rl_on_new_line();
                        rl_redisplay();
                }
                else if(ret == 0)
                {
                        // No child has completed yet.
                        break;
                }
                else
                {
                        if(errno == EINTR)
                                continue;

                        if(errno == ECHILD)
                                break;

                        break;
                }
        }
}
