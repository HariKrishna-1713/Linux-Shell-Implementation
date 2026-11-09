#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <signal.h>
#include "minishell.h"

#define GREEN "\033[92m"
#define BLUE  "\033[94m"
#define WHITE "\033[97m"
#define RESET "\033[0m"


// Handles pending signals and background process notifications.
// This function is called automatically by Readline through rl_event_hook.
int check_background_jobs(void)
{
        // Handle Ctrl+C (SIGINT) received while using the shell prompt.
        if(sigint_flag)
        {
                sigint_flag = 0;

                // Clear the current command line and redisplay the prompt.
                rl_replace_line("", 0);
                rl_crlf();
                rl_on_new_line();
                rl_redisplay();
        }

        // Handle background child process state changes (SIGCHLD).
        if(child_signal)
        {
                child_signal = 0;

                // Check and display the status of background jobs.
                repeated_background_jobs();
        }

        return 0;
}


int main()
{
        // Clear the terminal screen when the shell starts.
        system("clear");

        char cwd[MAX_INPUT], prompt[MAX_INPUT + 100], history_file[MAX_INPUT];
        char *ptr, *cmd;
        int status = 0;
        char *argv[MAX_ARGS + 1];
        int i, has_pipe, background;

        // Initialize Readline command history.
        using_history();

        // Construct the path for the shell history file.
        snprintf(history_file, sizeof(history_file),
                 "%s/.minishell_history", getenv("HOME"));

        // Load previously executed commands from the history file.
        read_history(history_file);

        // Allow Readline to handle terminal-related signals.
        rl_catch_signals = 1;

        // Register custom signal handlers.
        setup_signals();

        // Register check_background_jobs() as Readline's event hook.
        // It periodically checks pending signal flags while waiting for input.
        rl_event_hook = check_background_jobs;

        // Set keyboard input timeout to 100 milliseconds.
        rl_set_keyboard_input_timeout(100000);

        // Main shell loop: continuously accept and execute commands.
        while(1)
        {
                has_pipe = 0;
                background = 0;

                // Get the current working directory to construct the prompt.
                if(getcwd(cwd, sizeof(cwd)) != NULL)
                        snprintf(prompt, sizeof(prompt),
                                 GREEN "minishell:" RESET
                                 BLUE "%s~" RESET
                                 WHITE "$ " RESET, cwd);
                else
                        snprintf(prompt, sizeof(prompt),
                                 GREEN "minishell:~" RESET
                                 WHITE "$ " RESET);

                // Display the shell prompt and read user input.
                cmd = readline(prompt);

                // If readline returns NULL (usually Ctrl+D), exit the shell.
                if(cmd == NULL)
                {
                        printf("\n");
                        break;
                }

                // Skip leading spaces and tabs.
                ptr = cmd;
                while(*ptr == ' ' || *ptr == '\t')
                        ptr++;

                // Ignore empty commands.
                if(*ptr == '\0')
                        continue;

                // Check whether the user entered the exit command.
                if(strcmp(ptr, "exit") == 0)
                        break;

                // Add the entered command to Readline history.
                add_history(cmd);

                // Tokenize the command and store arguments in argv[].
                // Example: ls -l becomes argv[0] = "ls", argv[1] = "-l".
                parse_command(cmd, argv);


                // -------------------------------------------------
                // STEP 1: Check for background execution (&).
                // -------------------------------------------------

                for(int i = 0; argv[i] != NULL; i++)
                {
                        if(!strcmp(argv[i], "&"))
                        {
                                background = 1;

                                // Remove '&' from argv so execvp() does not receive it.
                                argv[i] = NULL;
                                break;
                        }
                }

                // If '&' is present, execute the command in the background.
                if(background)
                {
                        execute_background(argv);
                        continue;
                }


                // -------------------------------------------------
                // STEP 2: Check whether the command contains a pipe.
                // -------------------------------------------------

                for(int i = 0; argv[i] != NULL; i++)
                {
                        if(!strcmp(argv[i], "|"))
                        {
                                has_pipe = 1;
                                break;
                        }
                }

                // If a pipe is present, execute the pipeline.
                // Example: ls | grep .c
                if(has_pipe)
                {
                        status = execute_pipeline(argv);
                        continue;
                }


                // -------------------------------------------------
                // STEP 3: Check for built-in/custom commands.
                // -------------------------------------------------

                int result = custom_command(argv);

                // result == -1 or result == 0:
                // The command was handled internally or execution should stop.
                if(result == -1 || result == 0)
                        continue;


                // -------------------------------------------------
                // STEP 4: Execute external commands.
                // -------------------------------------------------

                // result == 1 means the command should be executed externally.
                if(result == 1)
                {
                        status = execute_command(argv);

                        if(status == -1)
                                continue;
                }
        }

        // Save command history before terminating the shell.
        write_history(history_file);

        return 0;
}
