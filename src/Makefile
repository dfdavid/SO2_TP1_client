CC=gcc
CFLAGS= -Werror -Wall -pedantic

all: tp1_client

tp1_client: main.o
	$(CC) -o tp1_client main.o

main.o: main.c
	cppcheck --enable=all --inconclusive --std=posix main.c
	$(CC) $(CFLAGS)  main.c -c

clean:
	rm -f tp1_client *.o
