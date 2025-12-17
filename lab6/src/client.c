#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"

struct Server {
  char ip[255];
  int port;
};

struct ClientTask {
  struct Server server;
  uint64_t begin;
  uint64_t end;
  uint64_t mod;
  uint64_t result;
  int ok;
};

static int ParseServerLine(const char *line, struct Server *s) {
  // line: "ip:port"
  char buf[512];
  snprintf(buf, sizeof(buf), "%s", line);

  // убрать \n
  size_t n = strlen(buf);
  while (n && (buf[n-1] == '\n' || buf[n-1] == '\r')) buf[--n] = '\0';

  char *colon = strchr(buf, ':');
  if (!colon) return 0;
  *colon = '\0';
  const char *ip = buf;
  const char *port_str = colon + 1;

  int port = atoi(port_str);
  if (port <= 0 || port > 65535) return 0;

  memset(s, 0, sizeof(*s));
  snprintf(s->ip, sizeof(s->ip), "%s", ip);
  s->port = port;
  return 1;
}

static struct Server *ReadServers(const char *path, unsigned int *out_n) {
  FILE *f = fopen(path, "r");
  if (!f) return NULL;

  unsigned int cap = 8;
  unsigned int n = 0;
  struct Server *arr = malloc(sizeof(struct Server) * cap);
  if (!arr) { fclose(f); return NULL; }

  char line[512];
  while (fgets(line, sizeof(line), f)) {
    struct Server s;
    if (!ParseServerLine(line, &s)) continue;
    if (n == cap) {
      cap *= 2;
      struct Server *tmp = realloc(arr, sizeof(struct Server) * cap);
      if (!tmp) { free(arr); fclose(f); return NULL; }
      arr = tmp;
    }
    arr[n++] = s;
  }

  fclose(f);
  *out_n = n;
  return arr;
}

static void *ServerWorker(void *arg) {
  struct ClientTask *t = (struct ClientTask *)arg;
  t->ok = 0;
  t->result = 0;

  struct hostent *hostname = gethostbyname(t->server.ip);
  if (!hostname) {
    fprintf(stderr, "gethostbyname failed: %s\n", t->server.ip);
    return NULL;
  }

  struct sockaddr_in server;
  server.sin_family = AF_INET;
  server.sin_port = htons(t->server.port);
  server.sin_addr.s_addr = *((unsigned long *)hostname->h_addr);

  int sck = socket(AF_INET, SOCK_STREAM, 0);
  if (sck < 0) {
    fprintf(stderr, "Socket creation failed!\n");
    return NULL;
  }

  if (connect(sck, (struct sockaddr *)&server, sizeof(server)) < 0) {
    fprintf(stderr, "Connection failed to %s:%d\n", t->server.ip, t->server.port);
    close(sck);
    return NULL;
  }

  char task[sizeof(uint64_t) * 3];
  memcpy(task, &t->begin, sizeof(uint64_t));
  memcpy(task + sizeof(uint64_t), &t->end, sizeof(uint64_t));
  memcpy(task + 2 * sizeof(uint64_t), &t->mod, sizeof(uint64_t));

  if (send(sck, task, sizeof(task), 0) < 0) {
    fprintf(stderr, "Send failed\n");
    close(sck);
    return NULL;
  }

  uint64_t answer = 0;
  char response[sizeof(uint64_t)];
  int rd = recv(sck, response, sizeof(response), 0);
  if (rd < 0 || rd != (int)sizeof(uint64_t)) {
    fprintf(stderr, "Receive failed\n");
    close(sck);
    return NULL;
  }

  memcpy(&answer, response, sizeof(uint64_t));
  t->result = answer;
  t->ok = 1;

  close(sck);
  return NULL;
}

int main(int argc, char **argv) {
  uint64_t k = (uint64_t)-1;
  uint64_t mod = (uint64_t)-1;
  char servers_path[255] = {'\0'};

  while (true) {
    static struct option options[] = {
        {"k", required_argument, 0, 0},
        {"mod", required_argument, 0, 0},
        {"servers", required_argument, 0, 0},
        {0, 0, 0, 0}};
    int option_index = 0;
    int c = getopt_long(argc, argv, "", options, &option_index);
    if (c == -1) break;

    if (c == 0) {
      if (option_index == 0) {
        if (!ConvertStringToUI64(optarg, &k) || k < 1) {
          fprintf(stderr, "k must be >= 1\n");
          return 1;
        }
      } else if (option_index == 1) {
        if (!ConvertStringToUI64(optarg, &mod) || mod < 1) {
          fprintf(stderr, "mod must be >= 1\n");
          return 1;
        }
      } else if (option_index == 2) {
        if (strlen(optarg) >= sizeof(servers_path)) {
          fprintf(stderr, "servers path too long\n");
          return 1;
        }
        strcpy(servers_path, optarg);
      }
    } else {
      fprintf(stderr, "Arguments error\n");
      return 1;
    }
  }

  if (k == (uint64_t)-1 || mod == (uint64_t)-1 || !strlen(servers_path)) {
    fprintf(stderr, "Using: %s --k 1000 --mod 5 --servers servers.txt\n", argv[0]);
    return 1;
  }

  unsigned int servers_num = 0;
  struct Server *servers = ReadServers(servers_path, &servers_num);
  if (!servers || servers_num == 0) {
    fprintf(stderr, "No servers found in file: %s\n", servers_path);
    return 1;
  }

  // Делим диапазон [1..k] на servers_num частей
  uint64_t len = k;
  uint64_t chunk = len / servers_num;
  uint64_t rem = len % servers_num;

  struct ClientTask *tasks = calloc(servers_num, sizeof(struct ClientTask));
  pthread_t *threads = calloc(servers_num, sizeof(pthread_t));

  uint64_t cur = 1;
  for (unsigned int i = 0; i < servers_num; i++) {
    uint64_t add = chunk + (i < rem ? 1 : 0);
    tasks[i].server = servers[i];
    tasks[i].begin = cur;
    tasks[i].end = cur + add - 1;
    tasks[i].mod = mod;
    cur += add;

    if (pthread_create(&threads[i], NULL, ServerWorker, (void *)&tasks[i])) {
      fprintf(stderr, "pthread_create failed\n");
      return 1;
    }
  }

  uint64_t total = 1;
  for (unsigned int i = 0; i < servers_num; i++) {
    pthread_join(threads[i], NULL);
    if (!tasks[i].ok) {
      fprintf(stderr, "Server %s:%d failed\n", tasks[i].server.ip, tasks[i].server.port);
      continue;
    }
    total = MultModulo(total, tasks[i].result, mod);
  }

  printf("answer: %llu\n", (unsigned long long)total);

  free(threads);
  free(tasks);
  free(servers);
  return 0;
}
