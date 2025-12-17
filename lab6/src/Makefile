CC=gcc
CFLAGS=-Wall -Wextra -g -O2 -pthread

all: client server

common.o: common.c common.h
	$(CC) $(CFLAGS) -c common.c -o common.o

client: client.c common.o common.h
	$(CC) $(CFLAGS) client.c common.o -o client

server: server.c common.o common.h
	$(CC) $(CFLAGS) server.c common.o -o server

clean:
	rm -f client server *.o
