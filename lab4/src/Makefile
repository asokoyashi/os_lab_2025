CC = gcc
CFLAGS = -Wall -Wextra -std=c99

all: parallel_min_max zombie process_memory

parallel_min_max: parallel_min_max.c find_min_max.o utils.o
	$(CC) $(CFLAGS) $^ -o $@

zombie: zombie.c
	$(CC) $(CFLAGS) $< -o $@

process_memory: process_memory.c
	$(CC) $(CFLAGS) $< -o $@

find_min_max.o: find_min_max.c find_min_max.h
	$(CC) $(CFLAGS) -c $<

utils.o: utils.c utils.h
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o parallel_min_max zombie process_memory

.PHONY: all clean
