#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

char *getoutput(const char *command) {
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        return NULL;
    }

    pid_t pid = fork();
    if (pid == -1) {
        close(pipefd[0]);
        close(pipefd[1]);
        return NULL;
    } else if (pid == 0) {
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);
        execl("/bin/sh", "sh", "-c", command, NULL);
        _exit(EXIT_FAILURE);
    } else {
        close(pipefd[1]);
        char *buf = malloc(4096);
        FILE *file_pointer = fdopen(pipefd[0], "r");
        size_t t = 4096;
        getdelim(&buf, &t, '\0', file_pointer);
        fclose(file_pointer);
        int status;
        waitpid(pid, &status, 0);
        return buf;
    }
}

char *parallelgetoutput(int count, const char *commands[][20]) {
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        return NULL;
    }

    pid_t pids[count];

    for (int i = 0; i < count; i++) {
        pids[i] = fork();
        if (pids[i] == 0) {
            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[0]);
            close(pipefd[1]);
            execvp(commands[i][0], (char *const *)commands[i]);
            perror("execvp");
            _exit(EXIT_FAILURE);
        }
    }

    close(pipefd[1]);

    char *output = NULL;
    size_t size = 0;
    ssize_t nread;
    char buffer[4096];
    while ((nread = read(pipefd[0], buffer, sizeof(buffer))) > 0) {
        output = realloc(output, size + nread + 1);
        memcpy(output + size, buffer, nread);
        size += nread;
    }
    output[size] = '\0';

    for (int i = 0; i < count; i++) {
        waitpid(pids[i], NULL, 0);
    }

    close(pipefd[0]);
    return output;
}
