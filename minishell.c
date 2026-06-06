#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_LINE 1024
#define MAX_ARGS 64
#define MAX_COMMANDS 16

void execute_pipeline(char ***cmds, int cmd_count) {
    int i;
    int pipefds[2 * (cmd_count - 1)];
    pid_t pids[MAX_COMMANDS];

    for (i = 0; i < cmd_count - 1; i++) {
        if (pipe(pipefds + i * 2) < 0) {
            perror("Pipe failed");
            exit(EXIT_FAILURE);
        }
    }

    for (i = 0; i < cmd_count; i++) {
        pids[i] = fork();
        if (pids[i] < 0) {
            perror("Fork failed");
            exit(EXIT_FAILURE);
        }

        if (pids[i] == 0) {
            if (i > 0) {
                if (dup2(pipefds[(i - 1) * 2], STDIN_FILENO) < 0) {
                    perror("dup2 stdin failed");
                    exit(EXIT_FAILURE);
                }
            }
            if (i < cmd_count - 1) {
                if (dup2(pipefds[i * 2 + 1], STDOUT_FILENO) < 0) {
                    perror("dup2 stdout failed");
                    exit(EXIT_FAILURE);
                }
            }

            for (int j = 0; j < 2 * (cmd_count - 1); j++) {
                close(pipefds[j]);
            }

            if (execvp(cmds[i][0], cmds[i]) < 0) {
                perror("Execution failed");
                exit(EXIT_FAILURE);
            }
        }
    }

    for (int j = 0; j < 2 * (cmd_count - 1); j++) {
        close(pipefds[j]);
    }

    for (i = 0; i < cmd_count; i++) {
        waitpid(pids[i], NULL, 0);
    }
}

int main() {
    char line[MAX_LINE];
    char *commands[MAX_COMMANDS];
    char **args_list[MAX_COMMANDS];

    for (int i = 0; i < MAX_COMMANDS; i++) {
        args_list[i] = malloc(MAX_ARGS * sizeof(char *));
    }

    while (1) {
        printf("minishell> ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) {
            break;
        }

        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }

        if (strcmp(line, "exit") == 0) {
            break;
        }

        if (strlen(line) == 0) {
            continue;
        }

        int cmd_count = 0;
        char *saveptr1 = NULL;
        char *saveptr2 = NULL;

        char *token = strtok_r(line, "|", &saveptr1);
        while (token != NULL && cmd_count < MAX_COMMANDS) {
            commands[cmd_count++] = token;
            token = strtok_r(NULL, "|", &saveptr1);
        }

        int valid_pipeline = 1;
        for (int i = 0; i < cmd_count; i++) {
            int arg_count = 0;
            char *arg = strtok_r(commands[i], " \t", &saveptr2);
            while (arg != NULL && arg_count < MAX_ARGS - 1) {
                args_list[i][arg_count++] = arg;
                arg = strtok_r(NULL, " \t", &saveptr2);
            }
            args_list[i][arg_count] = NULL;

            if (arg_count == 0) {
                valid_pipeline = 0;
            }
        }

        if (valid_pipeline && cmd_count > 0) {
            execute_pipeline(args_list, cmd_count);
        }
    }

    for (int i = 0; i < MAX_COMMANDS; i++) {
        free(args_list[i]);
    }

    return 0;
}
