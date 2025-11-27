#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <getopt.h>

#include "find_min_max.h"
#include "utils.h"

static int timeout_seconds = 0;

// Пустой обработчик — просто чтобы alarm() не убивал родителя
void alarm_handler(int sig) {
    // ничего не делаем, просто просыпаемся
}

int main(int argc, char **argv) {
    int seed = -1;
    int array_size = -1;
    int pnum = -1;
    bool with_files = false;

    while (true) {

        static struct option options[] = {
            {"seed",       required_argument, 0, 0},
            {"array_size", required_argument, 0, 0},
            {"pnum",       required_argument, 0, 0},
            {"by_files",   no_argument,       0, 'f'},
            {"timeout",    required_argument, 0, 't'},
            {0, 0, 0, 0}
        };

        int option_index = 0;
        int c = getopt_long(argc, argv, "ft:", options, &option_index);

        if (c == -1) break;

        switch (c) {
            case 0:
                switch (option_index) {
                    case 0: seed = atoi(optarg);       if (seed <= 0)       { printf("seed > 0\n");       return 1; } break;
                    case 1: array_size = atoi(optarg); if (array_size <= 0) { printf("array_size > 0\n"); return 1; } break;
                    case 2: pnum = atoi(optarg);       if (pnum <= 0)       { printf("pnum > 0\n");       return 1; } break;
                    case 3: with_files = true; break;
                    case 4: timeout_seconds = atoi(optarg);
                            if (timeout_seconds <= 0) { printf("timeout > 0\n"); return 1; }
                            break;
                }
                break;
            case 'f': with_files = true; break;
            case 't': timeout_seconds = atoi(optarg);
                      if (timeout_seconds <= 0) { printf("timeout > 0\n"); return 1; }
                      break;
            case '?': break;
            default:  printf("getopt error\n"); return 1;
        }
    }

    if (seed == -1 || array_size == -1 || pnum == -1) {
        printf("Usage: %s --seed N --array_size N --pnum N [--by_files] [--timeout N]\n", argv[0]);
        return 1;
    }

    int *array = malloc(sizeof(int) * array_size);
    GenerateArray(array, array_size, seed);

    struct timeval start_time;
    gettimeofday(&start_time, NULL);

    // Устанавливаем таймер, если задан
    if (timeout_seconds > 0) {
        signal(SIGALRM, alarm_handler);
        alarm(timeout_seconds);
    }

    int chunk_size = array_size / pnum;
    int active_child_processes = 0;
    int pipefd[pnum][2];
    FILE *files[pnum];

    for (int i = 0; i < pnum; i++) {
        int begin = i * chunk_size;
        int end = (i == pnum - 1) ? array_size : (i + 1) * chunk_size;

        if (with_files) {
            char filename[32];
            sprintf(filename, "min_max_%d.txt", i);
            files[i] = fopen(filename, "w");
        } else {
            pipe(pipefd[i]);
        }

        pid_t child_pid = fork();
        if (child_pid == 0) {
            struct MinMax mm = GetMinMax(array, begin, end);

            if (with_files) {
                fprintf(files[i], "%d %d", mm.min, mm.max);
                fclose(files[i]);
            } else {
                close(pipefd[i][0]);
                write(pipefd[i][1], &mm.min, sizeof(int));
                write(pipefd[i][1], &mm.max, sizeof(int));
                close(pipefd[i][1]);
            }
            free(array);
            exit(0);
        } else if (child_pid > 0) {
            active_child_processes++;
            if (!with_files) close(pipefd[i][1]);
        } else {
            printf("fork failed\n");
            return 1;
        }
    }

    // Ожидание детей + проверка таймаута
    while (active_child_processes > 0) {
        pid_t finished = waitpid(-1, NULL, WNOHANG);
        if (finished > 0) {
            active_child_processes--;
        } else if (finished == 0) {
            // никто не завершился
            if (timeout_seconds > 0 && alarm(0) == 0) {
                // alarm сработал — таймаут!
                printf("TIMEOUT! Killing all children...\n");
                kill(0, SIGKILL);  // убиваем всю группу процессов
                break;
            }
            usleep(100000);  // 0.1 сек, чтобы не грузить CPU
        }
    }

    // Отключаем alarm на всякий случай
    alarm(0);

    // Собираем результаты (как было раньше)
    struct MinMax global = {INT_MAX, INT_MIN};
    for (int i = 0; i < pnum; i++) {
        int min = INT_MAX, max = INT_MIN;
        if (with_files) {
            char filename[32];
            sprintf(filename, "min_max_%d.txt", i);
            FILE *f = fopen(filename, "r");
            if (f) {
                fscanf(f, "%d %d", &min, &max);
                fclose(f);
                remove(filename);
            }
        } else {
            read(pipefd[i][0], &min, sizeof(int));
            read(pipefd[i][0], &max, sizeof(int));
            close(pipefd[i][0]);
        }
        if (min < global.min) global.min = min;
        if (max > global.max) global.max = max;
    }

    struct timeval finish_time;
    gettimeofday(&finish_time, NULL);
    double elapsed = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
    elapsed += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

    free(array);

    printf("Min: %d\n", global.min);
    printf("Max: %d\n", global.max);
    printf("Elapsed time: %fms\n", elapsed);

    return 0;
}