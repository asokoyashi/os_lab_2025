#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    pid_t pid = fork();

    if (pid == 0) {
        // Дочерний процесс
        printf("Дочерний процесс (PID %d) завершился — стал зомби!\n", getpid());
        return 0;  // умирает, но родитель не делает wait → зомби
    } else if (pid > 0) {
        // Родительский процесс
        printf("Родитель (PID %d) спит 30 секунд. Открой htop/ps — увидишь зомби!\n", getpid());
        sleep(30);
        wait(NULL);  // теперь зомби исчезнет
        printf("Родитель проснулся и убрал зомби\n");
    } else {
        printf("fork не сработал!\n");
    }
    return 0;
}