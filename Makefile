CC=gcc
CFLAGS= -Werror -Wall -pedantic

all: programa_cliente

programa_cliente: main.o
	$(CC) -o programa_cliente main.o

main.o: main.c
	cppcheck --enable=all --inconclusive --std=posix main.c
	$(CC) $(CFLAGS)  main.c -c 

clean:
	rm -f programa_cliente *.o
