# Compiler
CC = gcc

# Compiler flags
CFLAGS = -pthread -Wall -Wextra

# Source files
SRCS = game_server.c game_client.c sock.c

# Object files
OBJS = $(SRCS:.c=.o)

# Executable names
SERVER = game_server
CLIENT = game_client

# Default target
all: $(SERVER) $(CLIENT)

# Rule to build the server executable
$(SERVER): sock.o game_server.o
	$(CC) $(CFLAGS) -o $@ $^

# Rule to build the client executable
$(CLIENT): sock.o game_client.o
	$(CC) $(CFLAGS) -o $@ $^

# Rule to compile sock.c
sock.o: sock.c sock.h
	$(CC) $(CFLAGS) -c sock.c

# Rule to compile game_server.c
game_server.o: game_server.c sock.h
	$(CC) $(CFLAGS) -c game_server.c

# Rule to compile game_client.c
game_client.o: game_client.c sock.h
	$(CC) $(CFLAGS) -c game_client.c

# Clean target to remove binaries and object files
clean:
	rm -f $(OBJS) $(SERVER) $(CLIENT)

# Phony targets
.PHONY: all clean
