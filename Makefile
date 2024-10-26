CC = gcc
CFLAGS = -Wall -Wextra
LIBS = -lraylib -lpthread

all: server client

server: server.c sock.c sock.h
	$(CC) $(CFLAGS) -o server server.c sock.c $(LIBS)

client: client.c sock.c sock.h
	$(CC) $(CFLAGS) -o client client.c sock.c $(LIBS)

clean:
	rm -f server client
