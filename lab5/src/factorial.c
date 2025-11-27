#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

int result = 1;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

struct FactorialArgs {
    int start;
    int end;
    int mod;
};

void* compute_factorial(void* arg) {
    struct FactorialArgs* args = (struct FactorialArgs*)arg;
    int local_result = 1;

    for (int i = args->start; i <= args->end; i++) {
        local_result = (local_result * i) % args->mod;
    }

    pthread_mutex_lock(&mutex);
    result = (result * local_result) % args->mod;
    pthread_mutex_unlock(&mutex);

    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc != 7) {
        printf("Usage: %s -k <number> --pnum <threads> --mod <mod>\n", argv[0]);
        return 1;
    }

    int k = atoi(argv[2]);
    int pnum = atoi(argv[4]);
    int mod = atoi(argv[6]);

    pthread_t threads[pnum];
    struct FactorialArgs args[pnum];
    int chunk_size = k / pnum;

    // Разбиваем работу между потоками
    for (int i = 0; i < pnum; i++) {
        args[i].start = i * chunk_size + 1;
        args[i].end = (i == pnum - 1) ? k : (i + 1) * chunk_size;
        args[i].mod = mod;
        pthread_create(&threads[i], NULL, compute_factorial, (void*)&args[i]);
    }

    for (int i = 0; i < pnum; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("Factorial of %d modulo %d is: %d\n", k, mod, result);
    pthread_mutex_destroy(&mutex);
    return 0;
}
