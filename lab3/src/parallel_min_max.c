#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <getopt.h>

#include "find_min_max.h"
#include "utils.h"

int main(int argc, char **argv) {
  int seed = -1;
  int array_size = -1;
  int pnum = -1;
  bool with_files = false;

  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {
        {"seed", required_argument, 0, 0},
        {"array_size", required_argument, 0, 0},
        {"pnum", required_argument, 0, 0},
        {"by_files", no_argument, 0, 'f'},
        {0, 0, 0, 0}
    };

    int option_index = 0;
    int c = getopt_long(argc, argv, "f", options, &option_index);

    if (c == -1) break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0: seed = atoi(optarg); 
            if (seed <= 0) { printf("seed must be positive\n"); return 1; }
            break;
          case 1: array_size = atoi(optarg); 
            if (array_size <= 0) { printf("array_size must be positive\n"); return 1; }
            break;
          case 2: pnum = atoi(optarg); 
            if (pnum <= 0) { printf("pnum must be positive\n"); return 1; }
            break;
          case 3: with_files = true; break;
          default:
            printf("Index %d is out of options\n", option_index);
        }
        break;
      case 'f':
        with_files = true;
        break;
      case '?':
        break;
      default:
        printf("getopt returned character code 0%o?\n", c);
    }
  }

  if (seed == -1 || array_size == -1 || pnum == -1) {
    printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" [--by_files]\n", argv[0]);
    return 1;
  }

  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);

  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  int chunk_size = array_size / pnum;
  int active_child_processes = 0;

  int pipefd[pnum][2];  // Для pipe
  FILE *files[pnum];    // Для файлов

  for (int i = 0; i < pnum; i++) {
    int begin = i * chunk_size;
    int end = (i == pnum - 1) ? array_size : (i + 1) * chunk_size;

    if (with_files) {
      char filename[32];
      sprintf(filename, "min_max_%d.txt", i);
      files[i] = fopen(filename, "w");
      if (!files[i]) {
        printf("Cannot open file %s\n", filename);
        return 1;
      }
    } else {
      if (pipe(pipefd[i]) == -1) {
        printf("pipe() failed\n");
        return 1;
      }
    }

    pid_t child_pid = fork();
    if (child_pid == 0) {
      // Дочерний процесс
      struct MinMax min_max = GetMinMax(array, begin, end);

      if (with_files) {
        fprintf(files[i], "%d %d", min_max.min, min_max.max);
        fclose(files[i]);
      } else {
        close(pipefd[i][0]);  // закрываем чтение
        write(pipefd[i][1], &min_max.min, sizeof(int));
        write(pipefd[i][1], &min_max.max, sizeof(int));
        close(pipefd[i][1]);
      }
      free(array);
      exit(0);
    } else if (child_pid > 0) {
      active_child_processes++;
      if (!with_files) close(pipefd[i][1]);  // родитель не пишет
    } else {
      printf("Fork failed!\n");
      return 1;
    }
  }

  // Ждём завершения всех детей
  while (active_child_processes > 0) {
    wait(NULL);
    active_child_processes--;
  }

  struct MinMax global_min_max;
  global_min_max.min = INT_MAX;
  global_min_max.max = INT_MIN;

  // Собираем результаты
  for (int i = 0; i < pnum; i++) {
    int min = INT_MAX;
    int max = INT_MIN;

    if (with_files) {
      char filename[32];
      sprintf(filename, "min_max_%d.txt", i);
      FILE *f = fopen(filename, "r");
      fscanf(f, "%d %d", &min, &max);
      fclose(f);
      remove(filename);
    } else {
      close(pipefd[i][1]);  // уже закрыт
      read(pipefd[i][0], &min, sizeof(int));
      read(pipefd[i][0], &max, sizeof(int));
      close(pipefd[i][0]);
    }

    if (min < global_min_max.min) global_min_max.min = min;
    if (max > global_min_max.max) global_min_max.max = max;
  }

  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  free(array);

  printf("Min: %d\n", global_min_max.min);
  printf("Max: %d\n", global_min_max.max);
  printf("Elapsed time: %fms\n", elapsed_time);

  return 0;
}