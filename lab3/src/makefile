CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -I.

TARGETS = sequential_min_max parallel_min_max launcher

all: $(TARGETS)

sequential_min_max: sequential_min_max.c find_min_max.o utils.o
	$(CC) $(CFLAGS) sequential_min_max.c find_min_max.o utils.o -o $@

parallel_min_max: parallel_min_max.c find_min_max.o utils.o
	$(CC) $(CFLAGS) parallel_min_max.c find_min_max.o utils.o -o $@

launcher: launcher.c
	$(CC) $(CFLAGS) launcher.c -o launcher

find_min_max.o: find_min_max.c find_min_max.h utils.h
	$(CC) $(CFLAGS) -c find_min_max.c

utils.o: utils.c utils.h
	$(CC) $(CFLAGS) -c utils.c

clean:
	rm -f *.o $(TARGETS)

.PHONY: all clean