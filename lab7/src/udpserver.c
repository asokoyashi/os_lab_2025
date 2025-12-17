#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <getopt.h>

#define SADDR struct sockaddr
#define SLEN sizeof(struct sockaddr_in)

static void Usage(const char *prog) {
  fprintf(stderr, "Usage: %s --port <port> --bufsize <n>\n", prog);
  fprintf(stderr, "Example: %s --port 20001 --bufsize 1024\n", prog);
}

int main(int argc, char **argv) {
  int port = -1;
  int bufsize = -1;

  while (1) {
    static struct option opts[] = {
        {"port", required_argument, 0, 0},
        {"bufsize", required_argument, 0, 0},
        {0, 0, 0, 0}};
    int idx = 0;
    int c = getopt_long(argc, argv, "", opts, &idx);
    if (c == -1) break;

    if (c == 0) {
      if (idx == 0) port = atoi(optarg);
      else if (idx == 1) bufsize = atoi(optarg);
    } else {
      Usage(argv[0]);
      return 1;
    }
  }

  if (port <= 0 || port > 65535 || bufsize <= 0) {
    Usage(argv[0]);
    return 1;
  }

  int sockfd, n;
  char *mesg = (char *)malloc((size_t)bufsize + 1);
  if (!mesg) {
    perror("malloc");
    return 1;
  }

  char ipadr[16];
  struct sockaddr_in servaddr;
  struct sockaddr_in cliaddr;

  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("socket problem");
    free(mesg);
    return 1;
  }

  memset(&servaddr, 0, SLEN);
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons((uint16_t)port);

  if (bind(sockfd, (SADDR *)&servaddr, SLEN) < 0) {
    perror("bind problem");
    close(sockfd);
    free(mesg);
    return 1;
  }

  printf("UDP SERVER starts on port %d (bufsize=%d)\n", port, bufsize);

  while (1) {
    unsigned int len = SLEN;

    n = (int)recvfrom(sockfd, mesg, (size_t)bufsize, 0, (SADDR *)&cliaddr, &len);
    if (n < 0) {
      perror("recvfrom");
      break;
    }
    mesg[n] = '\0';

    printf("REQUEST %s FROM %s : %d\n",
           mesg,
           inet_ntop(AF_INET, (void *)&cliaddr.sin_addr.s_addr, ipadr, 16),
           ntohs(cliaddr.sin_port));

    if (sendto(sockfd, mesg, (size_t)n, 0, (SADDR *)&cliaddr, len) < 0) {
      perror("sendto");
      break;
    }
  }

  close(sockfd);
  free(mesg);
  return 0;
}
