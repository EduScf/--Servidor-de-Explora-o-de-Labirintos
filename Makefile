CC = gcc
CFLAGS = -Wall

all: cliente servidor

comum.o: comum.c
	$(CC) $(CFLAGS) -c comum.c -o comum.o

cliente: cliente.c comum.o
	$(CC) $(CFLAGS) cliente.c comum.o -o cliente

servidor: servidor.c comum.o
	$(CC) $(CFLAGS) servidor.c comum.o -o servidor

clean:
	rm -f *.o cliente servidor
