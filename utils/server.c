#include "socket.h"
#include "server.h"
#include "common.h"
#include "error_type.h"

static response_t simple_server_response_functions[MAX_CLIENTS];
static int simple_server_client_sockfds[MAX_CLIENTS];
static pthread_t simple_server_worker_threads[MAX_CLIENTS];
static int n_client_sockfds = 0;

void *simple_server_worker(void *p_sockfd)
{
    int sockfd = *(int *)p_sockfd;
    response_t response_function = simple_server_response_functions[sockfd];
    int result = 0;
    signal(SIGPIPE, sigpipe_handler);
    while(true)
    {
        char req_buffer[DEFAULT_BUFFER_CAPACITY];
        int req_buffer_size = 0;
        result = recv_message(sockfd, req_buffer, &req_buffer_size, DEFAULT_BUFFER_CAPACITY);
        TEXIT_IF(IS_ERROR(result), , "Error: Could not receive message.\n");

        char res_buffer[DEFAULT_BUFFER_CAPACITY];
        int res_buffer_size = 0;
        result = response_function(sockfd, req_buffer, req_buffer_size, res_buffer, &res_buffer_size, DEFAULT_BUFFER_CAPACITY);
        TEXIT_IF(IS_ERROR(result), , "Error: Response error.\n");

        result = send_message(sockfd, res_buffer, res_buffer_size);
        TEXIT_IF(IS_ERROR(result), , "Error: Could not send message.\n");
    }
    pthread_exit(NULL);
}

int simple_server(int port, response_t response)
{
    int result;
    EXIT_IF(n_client_sockfds != 0, , "Error: Multiple servers.");
    int sockfd = create_socket();
    bind_socket(sockfd, port);
    listen_socket(sockfd);
    while (n_client_sockfds < MAX_CLIENTS)
    {
        int client_sockfd = wait_for_client(sockfd);
        simple_server_response_functions[client_sockfd] = response;
        simple_server_client_sockfds[n_client_sockfds] = client_sockfd;
        pthread_t thread;
        result = pthread_create(&thread, NULL, simple_server_worker, &client_sockfd);
        EXIT_IF(result != 0, close(sockfd), "Error: Could not create a new thread.");
        simple_server_worker_threads[n_client_sockfds] = thread;
        n_client_sockfds++;
    }
    close(sockfd);
    return 0;
}