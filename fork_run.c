#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

char *getoutput(const char *command) {
    int fds[2];
    if (pipe(fds) == -1) {
        return NULL;
    }
    pid_t childPid = fork();
    if (childPid == -1) {
        close(fds[0]);
        close(fds[1]);
        return NULL;
    } else if (childPid == 0) {
        dup2(fds[1], STDOUT_FILENO);
        close(fds[0]);
        close(fds[1]);
        execl("/bin/sh", "sh", "-c", command, NULL);
        _exit(EXIT_FAILURE);
    } else {
        close(fds[1]);
        FILE *stream = fdopen(fds[0], "r");
        char *buffer = malloc(4096);
        size_t bufferSize = 4096;
        getdelim(&buffer, &bufferSize, '\0', stream);
        fclose(stream);
        int status;
        waitpid(childPid, &status, 0);
        return buffer;
    }
}

char *parallelgetoutput(int numCommands, const char **commands) {
    int fds[2];
    pipe(fds);
    pid_t pidArray[numCommands];
    for (int i = 0; i < numCommands; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            close(fds[0]);
            char commandIndex[10];
            snprintf(commandIndex, sizeof(commandIndex), "%d", i);
            const char *args[10];
            int argIndex;
            for (argIndex = 0; commands[argIndex] != NULL; argIndex++) {
                args[argIndex] = commands[argIndex];
            }
            args[argIndex++] = commandIndex;
            args[argIndex] = NULL;
            dup2(fds[1], STDOUT_FILENO);
            close(fds[1]);
            execv(commands[0], (char * const *)args);
        } else {
            pidArray[i] = pid;
        }
    }

    close(fds[1]);
    for (int i = 0; i < numCommands; i++) {
        waitpid(pidArray[i], NULL, 0);
    }
    char *combinedOutput = NULL;
    size_t combinedSize = 0;
    char readBuffer[4096];
    ssize_t bytesRead;
    while ((bytesRead = read(fds[0], readBuffer, sizeof(readBuffer))) > 0) {
        combinedOutput = realloc(combinedOutput, combinedSize + bytesRead + 1);
        memcpy(combinedOutput + combinedSize, readBuffer, bytesRead);
        combinedSize += bytesRead;
    }
    close(fds[0]);
    combinedOutput[combinedSize] = '\0';
    return combinedOutput;
}
