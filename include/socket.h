#ifndef SOCKET_H
#define SOCKET_H

#include "std.h"

#define MAX_MSG_SIZE 10 * 1024 * 1024

#define MAX_READ_TIMES 1024

int create_socket();

void bind_socket(int sockfd, uint16_t port);

void listen_socket(int sockfd);

int wait_for_client(int sockfd);

void connect_to_server(int sockfd, const char* ip, uint16_t port);

int send_message(int sockfd, const char *buffer, int size);

int recv_message(int sockfd, char** p_buffer, int* p_size);

void sigpipe_handler(int sig);

#endif