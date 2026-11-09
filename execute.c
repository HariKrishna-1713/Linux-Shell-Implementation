#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<stdlib.h>
#include<sys/wait.h>
#include<fcntl.h>
#include<errno.h>
#include "minishell.h"

int handle_redirection(char *argv[])
{
        int i,j,fd,target;

        for(i=0; argv[i]!=NULL; i++)
        {
                // Identify input redirection, output redirection or append redirection.
                if((!strcmp(argv[i], "<")) ||
                   (!strcmp(argv[i], ">")) ||
                   (!strcmp(argv[i], ">>")))
                {
                        if((argv[i+1] == NULL))
                        {
                                fprintf(stderr, "minishell: missing filename\n");
                                return -1;
                        }

                        if((!strcmp(argv[i+1], "<")) ||
                           (!strcmp(argv[i+1], ">")) ||
                           (!strcmp(argv[i+1], ">>")))
                        {
                                fprintf(stderr, "Usage format: cmd redirection filename\n");
                                return -1;
                        }

                        // Open the file and select the standard file descriptor to redirect.
                        if(!strcmp(argv[i], "<"))
                        {
                                fd = open(argv[i+1], O_RDONLY);
                                target = STDIN_FILENO;
                        }
                        else if(!strcmp(argv[i], ">"))
                        {
                                fd = open(argv[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                                target = STDOUT_FILENO;
                        }
                        else if(!strcmp(argv[i], ">>"))
                        {
                                fd = open(argv[i+1], O_WRONLY | O_CREAT | O_APPEND, 0644);
                                target = STDOUT_FILENO;
                        }

                        if(fd == -1)
                        {
                                perror("open");
                                return -1;
                        }

                        // Redirect stdin/stdout to the opened file.
                        if(fd != target)
                        {
                                if(dup2(fd, target)==-1)
                                {
                                        perror("dup2");
                                        close(fd);
                                        return -1;
                                }
                                close(fd);
                        }

                        // Remove the redirection operator and filename from argv.
                        // Shift remaining arguments left so execvp() receives only command arguments.
                        for(j=i; argv[2+j] !=NULL; j++)
                                argv[j] = argv[j+2];

                        argv[j] = NULL;

                        // Recheck the current position for additional redirections.
                        i--;
                }
        }
        return 0;
}


int execute_command(char *argv[])
{
        int t,ret,status,i;

        // Create a child process to execute the external command.
        t = fork();

        if(t == -1)
        {
                perror("fork");
                return -1;
        }

        if(t == 0)
        {
                // Apply redirection in the child before executing the command.
                if(handle_redirection(argv) == -1)
                        exit(1);

                if(argv[0] == NULL)
                {
                        fprintf(stderr,"minishell: missing command\n");
                        exit(1);
                }

                // Replace the child process with the requested command.
                if((execvp(argv[0],argv))==-1)
                {
                        perror("execvp");
                        exit(2);
                }
        }
        else
        {
                foreground_running = 1;

                // Parent waits for the foreground child to finish.
                // Retry waitpid() if it is interrupted by a signal.
                while(waitpid(t,&status,0)==-1)
                {
                        if(errno == EINTR)
                        {
                                foreground_running = 0;
                                continue;
                        }

                        perror("waitpid");
                        return -1;
                }

                foreground_running = 0;
                return 0;
        }
}
