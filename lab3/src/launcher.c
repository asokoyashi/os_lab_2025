#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    pid_t pid = fork();

    if (pid == 0) {
        char *new_argv[] = {"sequential_min_max", "42", "100000", NULL};
        execvp("./sequential_min_max", new_argv);
        perror("execvp failed");
        exit(1);
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
        printf("Launcher: sequential_min_max завершен\n");
    } else {
        perror("fork failed");
    }
    return 0;
}