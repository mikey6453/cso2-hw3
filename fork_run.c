#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

static void executeCommand(const char *path, char *const argv[]) {
    execv(path, argv);
    perror("execv");
    _exit(EXIT_FAILURE);
}

char *parallelgetoutput(int count, const char ***cmdWithArgs) {
    int *pids = malloc(count * sizeof(pid_t));
    int **pipes = malloc(count * sizeof(int*));
    for (int i = 0; i < count; i++) {
        pipes[i] = malloc(2 * sizeof(int));
        if (pipe(pipes[i]) != 0) {
            perror("pipe");
            continue;
        }

        pid_t pid = fork();
        if (pid == 0) {
            close(pipes[i][0]);
            dup2(pipes[i][1], STDOUT_FILENO);
            executeCommand(cmdWithArgs[i][0], (char *const *)cmdWithArgs[i]);
        } else if (pid > 0) {
            close(pipes[i][1]);
            pids[i] = pid;
        } else {
            perror("fork");
        }
    }

    char *output = NULL;
    size_t totalSize = 0;
    for (int i = 0; i < count; i++) {
        waitpid(pids[i], NULL, 0);
        FILE *stream = fdopen(pipes[i][0], "r");
        if (stream == NULL) {
            perror("fdopen");
            continue;
        }

        fseek(stream, 0, SEEK_END);
        size_t size = ftell(stream);
        fseek(stream, 0, SEEK_SET);

        char *buffer = malloc(size + 1);
        if (buffer) {
            fread(buffer, 1, size, stream);
            buffer[size] = '\0';

            char *newOutput = realloc(output, totalSize + size + 1);
            if (!newOutput) {
                perror("realloc");
                free(buffer);
                break;
            }
            output = newOutput;
            memcpy(output + totalSize, buffer, size + 1);
            totalSize += size;
        }
        fclose(stream);
        close(pipes[i][0]);
        free(pipes[i]);
    }
    free(pipes);
    free(pids);

    return output;
}
