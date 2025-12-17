CC=gcc
CFLAGS=-Wall -Wextra -g

all: tcpserver tcpclient udpserver udpclient

tcpserver: tcpserver.c
	$(CC) $(CFLAGS) tcpserver.c -o tcpserver

tcpclient: tcpclient.c
	$(CC) $(CFLAGS) tcpclient.c -o tcpclient

udpserver: udpserver.c
	$(CC) $(CFLAGS) udpserver.c -o udpserver

udpclient: udpclient.c
	$(CC) $(CFLAGS) udpclient.c -o udpclient

clean:
	rm -f tcpserver tcpclient udpserver udpclient
