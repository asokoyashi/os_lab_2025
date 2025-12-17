#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>

#include <arpa/inet.h>
#include <getopt.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define SADDR struct sockaddr
#define SLEN sizeof(struct sockaddr_in)

static void Usage(const char *prog) {
  fprintf(stderr, "Usage: %s --ip <ip> --port <port> --bufsize <n>\n", prog);
  fprintf(stderr, "Example: %s --ip 127.0.0.1 --port 20001 --bufsize 1024\n", prog);
}

int main(int argc, char **argv) {
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

  int sockfd, n;
  char *sendline = (char *)malloc((size_t)bufsize);
  char *recvline = (char *)malloc((size_t)bufsize + 1);
  if (!sendline || !recvline) {
    perror("malloc");
    return 1;
  }

  struct sockaddr_in servaddr;

  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons((uint16_t)port);

  if (inet_pton(AF_INET, ip, &servaddr.sin_addr) <= 0) {
    perror("inet_pton problem");
    return 1;
  }

  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("socket problem");
    return 1;
  }

  write(1, "Enter string\n", 13);

  while ((n = (int)read(0, sendline, (size_t)bufsize)) > 0) {
    if (sendto(sockfd, sendline, (size_t)n, 0, (SADDR *)&servaddr, SLEN) == -1) {
      perror("sendto problem");
      break;
    }

    int rn = (int)recvfrom(sockfd, recvline, (size_t)bufsize, 0, NULL, NULL);
    if (rn == -1) {
      perror("recvfrom problem");
      break;
    }
    recvline[rn] = '\0';

    printf("REPLY FROM SERVER= %s\n", recvline);
  }

  close(sockfd);
  free(sendline);
  free(recvline);
  return 0;
}
