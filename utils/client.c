#include "client.h"
#include "std.h"
#include "socket.h"
#include "error_type.h"
#include "server.h"

static int client_sockfd;

int custom_client_response(const char *req_buffer, int req_size, char **p_res_buffer, int *p_res_size)
{
    int result = send_message(client_sockfd, req_buffer, req_size);
    RET_ERR_RESULT(result); 

    char *res_buffer = NULL;
    int res_size = 0;
    result = recv_message(client_sockfd, &res_buffer, &res_size);
    RET_ERR_RESULT(result); 

    *p_res_buffer = res_buffer;
    *p_res_size = res_size;
    return res_size;
}

void custom_client_init(const char *server_ip, int port, response_t *response)
{
    int sockfd = create_socket();
    connect_to_server(sockfd, server_ip, port);
    client_sockfd = sockfd;
    *response = custom_client_response;
}

void custom_client_close()
{
    close(client_sockfd);
}

void simple_client(const char *server_ip, int port, get_request_t get_request, handle_response_t handle_response)
{   
    response_t response;
    custom_client_init(server_ip, port, &response);

    int cycle = 0;
    int result;
    while (true)
    {
        char *req_buffer = NULL;
        int req_size = 0;
        result = get_request(&req_buffer, &req_size, cycle);
        EXIT_IF(IS_ERROR(result), custom_client_close(), "Error: Could not get request.\n");

        char *res_buffer = NULL;
        int res_size = 0;
        result = response(req_buffer, req_size, &res_buffer, &res_size);
        free(req_buffer);
        EXIT_IF(IS_ERROR(result), custom_client_close(), "Error: Could not get response.\n");
        
        result = handle_response(res_buffer, res_size, cycle);
        free(res_buffer);
        EXIT_IF(IS_ERROR(result), custom_client_close(), "Error: Could not receive response.\n");
        cycle++;
    }
    custom_client_close();
}