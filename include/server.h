#ifndef SERVER_H
#define SERVER_H

typedef int (*response_t)(const char*, int, char**, int*);

#define MAX_CLIENTS FD_SETSIZE

int simple_server(int port, response_t response);

#endif