CC = gcc

final: main.o parser.o custom_cmds.o execute.o pipeline.o jobs.o signal.o
        $(CC) main.o parser.o custom_cmds.o execute.o pipeline.o jobs.o signal.o -o minishell -lreadline

main.o: main.c minishell.h
        $(CC) -c main.c

parser.o: parser.c minishell.h
        $(CC) -c parser.c

custom_cmds.o: custom_cmds.c minishell.h
        $(CC) -c custom_cmds.c

execute.o: execute.c minishell.h
        $(CC) -c execute.c

pipeline.o: pipeline.c minishell.h
        $(CC) -c pipeline.c

jobs.o: jobs.c minishell.h
        $(CC) -c jobs.c

signal.o: signal.c minishell.h
        $(CC) -c signal.c

clean:
        rm *.o final

.PHONY: clean