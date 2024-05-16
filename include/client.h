#ifndef CLIENT_H
#define CLIENT_H
#include "server.h"

typedef int (*get_request_t)(char **req_buffer, int *req_size, int cycle);

typedef int (*handle_response_t)(const char *res_buffer, int res_size, int cycle);

void simple_client(const char *server_ip, int port, get_request_t get_request, handle_response_t handle_response);

void custom_client_init(const char *server_ip, int port, response_t *response);

void custom_client_close();

#endif