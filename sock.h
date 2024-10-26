/**
 * @file sock.h
 * @brief Socket API for client-server communication
 *
 * This header file provides a simple socket API for creating server and client
 * sockets, as well as sending and receiving data over these sockets.
 */

#ifndef SOCK_H
#define SOCK_H

#include <netinet/in.h>

/** Maximum size of the buffer for sending/receiving data */
#define BUFFER_SIZE 1024

/** Default port number for socket communication */
#define PORT 5000

/**
 * @brief Initialize a server socket
 *
 * This function creates and binds a server socket, then sets it to listen mode.
 *
 * @return The socket file descriptor on success, or -1 on error
 */
int init_server_socket();

/**
 * @brief Initialize a client socket and connect to a server
 *
 * This function creates a client socket and connects it to the specified server IP address.
 *
 * @param server_ip The IP address of the server to connect to
 * @return The socket file descriptor on success, or -1 on error
 */
int init_client_socket(const char* server_ip);

/**
 * @brief Send data over a socket
 *
 * This function sends the provided data over the specified socket.
 *
 * @param socket The socket file descriptor to send data through
 * @param data The null-terminated string to send
 * @return The number of bytes sent on success, or -1 on error
 */
int send_data(int socket, const char* data);

/**
 * @brief Receive data from a socket
 *
 * This function receives data from the specified socket and stores it in the provided buffer.
 *
 * @param socket The socket file descriptor to receive data from
 * @param buffer The buffer to store the received data
 * @param buffer_size The size of the buffer
 * @return The number of bytes received on success, or -1 on error
 */
int receive_data(int socket, char* buffer, int buffer_size);

#endif // SOCK_H