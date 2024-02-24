#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

static char *readOutputFromPipe(int fd) {
    char *output = NULL;
    size_t size = 0, capacity = 4096;
    ssize_t bytes_read;
    output = (char *)malloc(capacity);
    if (!output) return NULL;

    while ((bytes_read = read(fd, output + size, capacity - size - 1)) > 0) {
        size += bytes_read;
        if (size >= capacity - 1) {
            capacity *= 2;
            char *newOutput = realloc(output, capacity);
            if (!newOutput) {
                free(output);
                return NULL;
            }
            output = newOutput;
        }
    }
    output[size] = '\0';
    return output;
}

char *getoutput(const char *command) {
    int pipefd[2];
    if (pipe(pipefd) != 0) return NULL;

    pid_t pid = fork();
    if (pid == -1) {
        close(pipefd[0]);
        close(pipefd[1]);
        return NULL;
    } else if (pid == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        execl("/bin/sh", "sh", "-c", command, NULL);
        _exit(EXIT_FAILURE);
    }

    close(pipefd[1]);
    char *output = readOutputFromPipe(pipefd[0]);
    close(pipefd[0]);
    waitpid(pid, NULL, 0);
    return output;
}

char *parallelgetoutput(int count, const char **commands) {
    int *pids = malloc(count * sizeof(pid_t));
    int **pipes = malloc(count * sizeof(int*));
    char *output = NULL;
    size_t totalSize = 0;

    for (int i = 0; i < count; i++) {
        pipes[i] = malloc(2 * sizeof(int));
        if (pipe(pipes[i]) != 0) continue;
        
        pid_t pid = fork();
        if (pid == 0) {
            close(pipes[i][0]);
            dup2(pipes[i][1], STDOUT_FILENO);
            execl("/bin/sh", "sh", "-c", commands[i], NULL);
            _exit(EXIT_FAILURE);
        } else if (pid > 0) {
            close(pipes[i][1]);
            pids[i] = pid;
        }
    }

    for (int i = 0; i < count; i++) {
        char *cmdOutput = readOutputFromPipe(pipes[i][0]);
        if (cmdOutput) {
            size_t outputLen = strlen(cmdOutput);
            char *newOutput = realloc(output, totalSize + outputLen + 1);
            if (!newOutput) {
                free(output);
                free(cmdOutput);
                break;
            }
            output = newOutput;
            memcpy(output + totalSize, cmdOutput, outputLen);
            totalSize += outputLen;
            output[totalSize] = '\0';
            free(cmdOutput);
        }
        close(pipes[i][0]);
        free(pipes[i]);
        waitpid(pids[i], NULL, 0);
    }

    free(pipes);
    free(pids);
    return output;
}
