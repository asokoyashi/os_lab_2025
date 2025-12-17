#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <getopt.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <pthread.h>

#include "common.h"

struct FactorialArgs {
  uint64_t begin;
  uint64_t end;
  uint64_t mod;
};

static uint64_t Factorial(const struct FactorialArgs *args) {
  uint64_t ans = 1;
  for (uint64_t i = args->begin; i <= args->end; i++) {
    ans = MultModulo(ans, i % args->mod, args->mod);
  }
  return ans;
}

static void *ThreadFactorial(void *args) {
  struct FactorialArgs *fargs = (struct FactorialArgs *)args;
  uint64_t *res = (uint64_t *)malloc(sizeof(uint64_t));
  if (!res) pthread_exit(NULL);
  *res = Factorial(fargs);
  return (void *)res;
}

int main(int argc, char **argv) {
  int tnum = -1;
  int port = -1;

  while (true) {
    static struct option options[] = {
        {"port", required_argument, 0, 0},
        {"tnum", required_argument, 0, 0},
        {0, 0, 0, 0}};
    int option_index = 0;
    int c = getopt_long(argc, argv, "", options, &option_index);
    if (c == -1) break;

    if (c == 0) {
      if (option_index == 0) {
        port = atoi(optarg);
        if (port <= 0 || port > 65535) {
          fprintf(stderr, "port must be in (1..65535)\n");
          return 1;
        }
      } else if (option_index == 1) {
        tnum = atoi(optarg);
        if (tnum <= 0 || tnum > 256) {
          fprintf(stderr, "tnum must be in (1..256)\n");
          return 1;
        }
      }
    } else {
      fprintf(stderr, "Arguments error\n");
      return 1;
    }
  }

  if (port == -1 || tnum == -1) {
    fprintf(stderr, "Using: %s --port 20001 --tnum 4\n", argv[0]);
    return 1;
  }

  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    fprintf(stderr, "Can not create server socket!\n");
    return 1;
  }

  struct sockaddr_in server;
  server.sin_family = AF_INET;
  server.sin_port = htons((uint16_t)port);
  server.sin_addr.s_addr = htonl(INADDR_ANY);

  int opt_val = 1;
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(opt_val));

  int err = bind(server_fd, (struct sockaddr *)&server, sizeof(server));
  if (err < 0) {
    fprintf(stderr, "Can not bind to socket!\n");
    return 1;
  }

  err = listen(server_fd, 128);
  if (err < 0) {
    fprintf(stderr, "Could not listen on socket\n");
    return 1;
  }

  printf("Server listening at %d\n", port);

  while (true) {
    struct sockaddr_in client;
    socklen_t client_len = sizeof(client);
    int client_fd = accept(server_fd, (struct sockaddr *)&client, &client_len);
    if (client_fd < 0) {
      fprintf(stderr, "Could not establish new connection\n");
      continue;
    }

    while (true) {
      unsigned int buffer_size = sizeof(uint64_t) * 3;
      char from_client[buffer_size];
      int rd = recv(client_fd, from_client, buffer_size, 0);

      if (!rd) break;
      if (rd < 0) { fprintf(stderr, "Client read failed\n"); break; }
      if ((unsigned int)rd < buffer_size) { fprintf(stderr, "Wrong format\n"); break; }

      uint64_t begin = 0, end = 0, mod = 0;
      memcpy(&begin, from_client, sizeof(uint64_t));
      memcpy(&end, from_client + sizeof(uint64_t), sizeof(uint64_t));
      memcpy(&mod, from_client + 2 * sizeof(uint64_t), sizeof(uint64_t));

      fprintf(stdout, "Receive: %llu %llu %llu\n",
              (unsigned long long)begin,
              (unsigned long long)end,
              (unsigned long long)mod);

      uint64_t len = end - begin + 1;
      uint64_t chunk = len / (uint64_t)tnum;
      uint64_t rem = len % (uint64_t)tnum;

      pthread_t threads[tnum];
      struct FactorialArgs args[tnum];

      uint64_t cur = begin;
      for (int i = 0; i < tnum; i++) {
        uint64_t add = chunk + (i < (int)rem ? 1 : 0);
        args[i].begin = cur;
        args[i].end = cur + add - 1;
        args[i].mod = mod;
        cur += add;

        if (pthread_create(&threads[i], NULL, ThreadFactorial, (void *)&args[i])) {
          fprintf(stderr, "pthread_create failed!\n");
          close(client_fd);
          return 1;
        }
      }

      uint64_t total = 1;
      for (int i = 0; i < tnum; i++) {
        uint64_t *part = NULL;
        pthread_join(threads[i], (void **)&part);
        if (!part) continue;
        total = MultModulo(total, *part, mod);
        free(part);
      }

      printf("Total: %llu\n", (unsigned long long)total);

      char out[sizeof(uint64_t)];
      memcpy(out, &total, sizeof(uint64_t));
      err = send(client_fd, out, sizeof(uint64_t), 0);
      if (err < 0) { fprintf(stderr, "Can't send\n"); break; }
    }

    shutdown(client_fd, SHUT_RDWR);
    close(client_fd);
  }

  return 0;
}
