#include <stdio.h>
#include <string.h>
#include "minishell.h"

void parse_command(char *cmd, char *argv[])
{
        int i = 0, has_pipe = 0;
        char *p;

        // Initialize argv[0] to NULL.
        // This ensures argv is initially empty.
        argv[0] = NULL;

        // Split the command into tokens using space as the delimiter.
        // strtok() returns the first token from cmd.
        for(p = strtok(cmd, " "); p != NULL; p = strtok(NULL, " "))
        {
                // Check whether the maximum argument limit is reached.
                if(i >= MAX_ARGS)
                {
                        printf("Arguments limit is reached\n");

                        // Clear argv[0] to indicate invalid/empty arguments.
                        argv[0] = NULL;
                }

                // Store the current token address in argv[].
                // Example: ls -l
                // argv[0] = "ls"
                // argv[1] = "-l"
                argv[i] = p;

                // Move to the next argv[] position.
                i++;
        }

        // Mark the end of the argument list with NULL.
        // execvp() uses this NULL pointer to identify the end of argv[].
        argv[i] = NULL;


/*
        // Debugging code:
        // Print all tokens stored inside argv[].

        i = 0;

        while(argv[i] != NULL)
        {
                printf("%s\n", argv[i]);
                i++;
        }
*/
}
