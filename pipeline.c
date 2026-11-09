#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <errno.h>
#include "minishell.h"

#define MAX_CMDS 100

int execute_pipeline(char *argv[])
{
    char **commands[MAX_CMDS];
    int fd[MAX_CMDS - 1][2];
    pid_t pid[MAX_CMDS];

    int argc = 0;
    int ncmd = 1;
    int prev_was_pipe = 0;
    int created = 0;
    int i, j;
    int status;
    int last_status = 0;

    // Count the total number of arguments in argv.
    while(argv[argc] != NULL)
        argc++;

    // Split the original argv into separate commands using '|'.
    commands[0] = argv;

    for(i = 0; i < argc; i++)
    {
        if(!strcmp(argv[i], "|"))
        {
            // Validate pipe syntax: reject leading, trailing and consecutive pipes.
            if(i == 0 || i == argc - 1 ||
               !strcmp(argv[i + 1], "|"))
            {
                fprintf(stderr, "minishell: invalid pipe syntax\n");
                return -1;
            }

            if(ncmd >= MAX_CMDS)
            {
                fprintf(stderr, "minishell: too many commands\n");
                return -1;
            }

            // Replace '|' with NULL to terminate the current command.
            // Store the address of the next command in commands[].
            argv[i] = NULL;
            commands[ncmd++] = &argv[i + 1];

        }
    }

    // Create N-1 pipes for N commands.
    for(i = 0; i < ncmd - 1; i++)
    {
        if(pipe(fd[i]) == -1)
        {
            perror("pipe");

            // Close previously created pipes if pipe creation fails.
            for(j = 0; j < i; j++)
            {
                close(fd[j][0]);
                close(fd[j][1]);
            }

            return -1;
        }
    }

    // Create one child process for each command.
    for(i = 0; i < ncmd; i++)
    {
        pid[i] = fork();

        if(pid[i] == -1)
        {
            perror("fork");
            break;
        }

        if(pid[i] == 0)
        {
            // Read input from the previous pipe, if this is not the first command.
            if(i > 0)
            {
                if(dup2(fd[i - 1][0], STDIN_FILENO) == -1)
                {
                    perror("dup2 stdin");
                    exit(1);
                }
            }

            // Send output to the next pipe, if this is not the last command.
            if(i < ncmd - 1)
            {
                if(dup2(fd[i][1], STDOUT_FILENO) == -1)
                {
                    perror("dup2 stdout");
                    exit(1);
                }
            }

            // Close original pipe descriptors after dup2().
            // The required stdin/stdout connections remain open.
            for(j = 0; j < ncmd - 1; j++)
            {
                close(fd[j][0]);
                close(fd[j][1]);
            }

            // Apply any input/output redirection specified for this command.
            if(handle_redirection(commands[i]) == -1)
            {
                exit(1);
            }

            if(commands[i][0] == NULL)
            {
                fprintf(stderr, "minishell: missing command\n");
                exit(1);
            }

            // Replace the child process with the corresponding pipeline command.
            if((execvp(commands[i][0], commands[i]))== -1)
            {
                perror("execvp");
                exit(1);
            }
        }

        created++;
    }

    // Parent closes all pipe descriptors because it does not use the pipes.
    for(i = 0; i < ncmd - 1; i++)
    {
        close(fd[i][0]);
        close(fd[i][1]);
    }

    // Wait for all successfully created child processes to finish.
    for(i = 0; i < created; i++)
    {
        if(waitpid(pid[i], NULL, 0) == -1)
        {
                perror("waitpid");
                return -1;
        }
    }

    if(created != ncmd)
        return -1;

    return 0;
}
