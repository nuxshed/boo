# Compiler
CC = gcc

# Compiler flags
CFLAGS = -pthread -Wall -Wextra `pkg-config --cflags raylib` -std=c99

# Linker flags
LDFLAGS = `pkg-config --libs raylib` -lm

# Source files
SRCS = game_server.c sock.c game_client.c

# Object files
OBJS = game_server.o sock.o game_client.o

# Executable names
SERVER = game_server
CLIENT = game_client

# Default target
all: $(SERVER) $(CLIENT)

# Rule to build the server executable
$(SERVER): game_server.o sock.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Rule to build the client executable
$(CLIENT): game_client.o sock.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Rule to compile game_server.c
game_server.o: game_server.c sock.h
	$(CC) $(CFLAGS) -c game_server.c

# Rule to compile game_client.c
game_client.o: game_client.c sock.h
	$(CC) $(CFLAGS) -c game_client.c

# Rule to compile sock.c
sock.o: sock.c sock.h
	$(CC) $(CFLAGS) -c sock.c

# Clean target to remove binaries and object files
clean:
	rm -f $(OBJS) $(SERVER) $(CLIENT)

# Phony targets
.PHONY: all clean