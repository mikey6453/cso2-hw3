#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

char *captureCommandOutput(const char *command) {
    int pipefd[2];
    if (pipe(pipefd) != 0) {
        perror("pipe");
        return NULL;
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        close(pipefd[0]);
        close(pipefd[1]);
        return NULL;
    } else if (pid == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        execl("/bin/sh", "sh", "-c", command, NULL);
        perror("execl");
        _exit(EXIT_FAILURE);
    }

    close(pipefd[1]);
    char *output = NULL;
    size_t size = 0, capacity = 4096;
    output = (char *)malloc(capacity);
    if (!output) {
        perror("malloc");
        close(pipefd[0]);
        waitpid(pid, NULL, 0);
        return NULL;
    }

    ssize_t bytes_read;
    while ((bytes_read = read(pipefd[0], output + size, capacity - size - 1)) > 0) {
        size += bytes_read;
        if (size >= capacity - 1) {
            capacity *= 2;
            output = (char *)realloc(output, capacity);
            if (!output) {
                perror("realloc");
                close(pipefd[0]);
                waitpid(pid, NULL, 0);
                return NULL;
            }
        }
    }
    output[size] = '\0';
    close(pipefd[0]);
    waitpid(pid, NULL, 0);
    return output;
}

char *captureParallelCommandsOutput(int count, const char **commands) {
    char *output = NULL;
    size_t totalSize = 0;

    for (int i = 0; i < count; i++) {
        char *cmdOutput = captureCommandOutput(commands[i]);
        if (cmdOutput) {
            size_t outputLen = strlen(cmdOutput);
            char *newOutput = realloc(output, totalSize + outputLen + 1);
            if (!newOutput) {
                perror("realloc");
                free(output);
                free(cmdOutput);
                return NULL;
            }
            output = newOutput;
            memcpy(output + totalSize, cmdOutput, outputLen);
            totalSize += outputLen;
            output[totalSize] = '\0';
            free(cmdOutput);
        }
    }

    return output;
}
