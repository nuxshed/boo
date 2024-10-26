// sock.h
#ifndef SOCK_H
#define SOCK_H

#include <netinet/in.h>

#define BUFFER_SIZE 1024
#define PORT 5000

// Socket API
int init_server_socket();
int init_client_socket(const char* server_ip);
int send_data(int socket, const char* data);
int receive_data(int socket, char* buffer, int buffer_size);

#endif
