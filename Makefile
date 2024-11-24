CC = gcc
CFLAGS = -Wall

all: cliente servidor servidor-mt

comum.o: comum.c
	$(CC) $(CFLAGS) -c comum.c -o comum.o

cliente: cliente.c comum.o
	$(CC) $(CFLAGS) cliente.c comum.o -o cliente

servidor: servidor.c comum.o
	$(CC) $(CFLAGS) servidor.c comum.o -o servidor

servidor-mt: servidor-mt.c comum.o
	$(CC) $(CFLAGS) servidor-mt.c comum.o -o servidor-mt -lpthread

clean:
	rm -f *.o cliente servidor servidor-mt
