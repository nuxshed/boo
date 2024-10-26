#ifndef SOCK_H
#define SOCK_H

#include <stdbool.h>
#include <netinet/in.h>  // For sockaddr_in

#define MAX_CLIENTS 10
#define BUFFER_SIZE 256

// Structure representing a client connection
typedef struct {
    int sockfd;
    struct sockaddr_in addr;
} Connection;

// Structure representing the server with a list of clients
typedef struct {
    Connection clients[MAX_CLIENTS];
    int client_count;
    int server_fd;
} Server;

/**
 * Initializes the server on the specified IP address and port.
 * @param server Pointer to the Server structure.
 * @param ip_address The IP address to bind the server (e.g., "192.168.1.10").
 * @param port The port number to listen on.
 * @return True if successful, false otherwise.
 */
bool init_server(Server *server, const char *ip_address, int port);

/**
 * Accepts a new client connection.
 * @param server Pointer to the Server structure.
 * @return True if a client is successfully accepted, false otherwise.
 */
bool accept_client(Server *server);

/**
 * Sends a message to a specific client.
 * @param client Pointer to the Connection structure of the client.
 * @param message The message to send.
 * @return True if the message is sent successfully, false otherwise.
 */
bool send_to_client(const Connection *client, const char *message);

/**
 * Broadcasts a message to all clients except the sender.
 * @param server Pointer to the Server structure.
 * @param message The message to broadcast.
 * @param sender_index Index of the sender in the clients array (use -1 to send to all).
 */
void broadcast_to_clients(Server *server, const char *message, int sender_index);

/**
 * Receives a message from a specific client.
 * @param client Pointer to the Connection structure of the client.
 * @param buffer The buffer to store the received message.
 * @param buffer_size The size of the buffer.
 * @return The number of bytes received, or -1 on failure.
 */
int receive_from_client(const Connection *client, char *buffer, int buffer_size);

/**
 * Closes a specific client connection.
 * @param client Pointer to the Connection structure of the client.
 */
void close_client(Connection *client);

/**
 * Shuts down the server and closes all client connections.
 * @param server Pointer to the Server structure.
 */
void shutdown_server(Server *server);

#endif // SOCK_H
