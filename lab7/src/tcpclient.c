#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <getopt.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define SADDR struct sockaddr
#define SIZE sizeof(struct sockaddr_in)

static void Usage(const char *prog) {
  fprintf(stderr, "Usage: %s --ip <ip> --port <port> --bufsize <n>\n", prog);
  fprintf(stderr, "Example: %s --ip 127.0.0.1 --port 10050 --bufsize 100\n", prog);
}

int main(int argc, char *argv[]) {
  const char *ip = NULL;
  int port = -1;
  int bufsize = -1;

  while (1) {
    static struct option opts[] = {
        {"ip", required_argument, 0, 0},
        {"port", required_argument, 0, 0},
        {"bufsize", required_argument, 0, 0},
        {0, 0, 0, 0}};
    int idx = 0;
    int c = getopt_long(argc, argv, "", opts, &idx);
    if (c == -1) break;

    if (c == 0) {
      if (idx == 0) ip = optarg;
      else if (idx == 1) port = atoi(optarg);
      else if (idx == 2) bufsize = atoi(optarg);
    } else {
      Usage(argv[0]);
      return 1;
    }
  }

  if (!ip || port <= 0 || port > 65535 || bufsize <= 0) {
    Usage(argv[0]);
    return 1;
  }

  int fd;
  int nread;
  char *buf = (char *)malloc((size_t)bufsize);
  if (!buf) {
    perror("malloc");
    return 1;
  }

  struct sockaddr_in servaddr;

  if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket creating");
    free(buf);
    return 1;
  }

  memset(&servaddr, 0, SIZE);
  servaddr.sin_family = AF_INET;

  if (inet_pton(AF_INET, ip, &servaddr.sin_addr) <= 0) {
    perror("bad address");
    close(fd);
    free(buf);
    return 1;
  }

  servaddr.sin_port = htons((uint16_t)port);

  if (connect(fd, (SADDR *)&servaddr, SIZE) < 0) {
    perror("connect");
    close(fd);
    free(buf);
    return 1;
  }

  write(1, "Input message to send\n", 22);
  while ((nread = (int)read(0, buf, (size_t)bufsize)) > 0) {
    if (write(fd, buf, (size_t)nread) < 0) {
      perror("write");
      close(fd);
      free(buf);
      return 1;
    }
  }

  close(fd);
  free(buf);
  return 0;
}
