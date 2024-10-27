#ifndef SOCK_H
#define SOCK_H

#include <netinet/in.h>

#define BUFFER_SIZE 2048
#define DEFAULT_PORT 5000

/**
 * @brief Initialize a server socket.
 *
 * @param port The port number to bind the server.
 * @return The server socket file descriptor.
 */
int init_server_socket(int port);

/**
 * @brief Initialize a client socket and connect to the server.
 *
 * @param server_ip The IP address of the server.
 * @param port The port number to connect to.
 * @return The client socket file descriptor.
 */
int init_client_socket(const char* server_ip, int port);

/**
 * @brief Send data over a socket.
 *
 * @param socket The socket file descriptor.
 * @param data The data string to send.
 * @return The number of bytes sent, or -1 on failure.
 */
int send_data(int socket, const char* data);

/**
 * @brief Receive data from a socket.
 *
 * @param socket The socket file descriptor.
 * @param buffer The buffer to store received data.
 * @param buffer_size The size of the buffer.
 * @return The number of bytes received, or -1 on failure.
 */
int receive_data(int socket, char* buffer, int buffer_size);

#endif // SOCK_H
