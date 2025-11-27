CC = gcc
CFLAGS = -Wall -g -pthread

all: parallel_min_max parallel_sum process_memory zombie

parallel_min_max: parallel_min_max.c find_min_max.c utils.c
	$(CC) $(CFLAGS) parallel_min_max.c find_min_max.c utils.c -o parallel_min_max

parallel_sum: parallel_sum.c utils.c
	$(CC) $(CFLAGS) parallel_sum.c utils.c -o parallel_sum

process_memory: process_memory.c
	$(CC) $(CFLAGS) process_memory.c -o process_memory

zombie: zombie.c
	$(CC) $(CFLAGS) zombie.c -o zombie

clean:
	rm -f parallel_min_max parallel_sum process_memory zombie
