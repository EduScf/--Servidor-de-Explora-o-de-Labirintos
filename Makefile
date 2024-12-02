CC = gcc
CFLAGS = -Wall

all: client server servidor-mt

common.o: common.c
	$(CC) $(CFLAGS) -c common.c -o common.o

client: client.c common.o
	$(CC) $(CFLAGS) client.c common.o -o client

server: server.c common.o
	$(CC) $(CFLAGS) server.c common.o -o server

servidor-mt: servidor-mt.c common.o
	$(CC) $(CFLAGS) servidor-mt.c common.o -o servidor-mt -lpthread

clean:
	rm -f *.o client server servidor-mt
