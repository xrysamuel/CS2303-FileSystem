#ifndef SERVER_H
#define SERVER_H

typedef int (*response_t)(int sockfd, const char *req_buffer, int req_size, char *res_buffer, int *p_res_size, int max_res_size);

#define MAX_CLIENTS FD_SETSIZE
#define DEFAULT_MAX_MESSAGE_LEN 16384

int simple_server(int port, response_t response);

#endif