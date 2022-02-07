CC = clang
CFLAGS = -Wall -Wextra -Wpedantic -std=c11 -ggdb

kilo: kilo.o
	$(CC) $(CFLAGS) -o $@ $^
kilo.o: kilo.c
	$(CC) $(CFLAGS) -c -o $@ $^
