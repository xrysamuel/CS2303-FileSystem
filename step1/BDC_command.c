#include "std.h"
#include "socket.h"
#include "buffer.h"
#include "error_type.h"

#define BUFFER_SIZE 1048576

bool hex_mode = false;

int main(int argc, char *argv[])
{

    EXIT_IF(argc != 3, , "Usage: %s <server_ip> <port>\n", argv[0]);

    const char *server_ip = argv[1];
    int port = atoi(argv[2]);

    int result;
    int sockfd = create_socket();
    connect_to_server(sockfd, server_ip, port);

    char req_buffer[BUFFER_SIZE];
    int message_size;
    while (true)
    {
        printf("> ");
        message_size = readline_to_buffer(req_buffer, BUFFER_SIZE);
        if (starts_with(req_buffer, BUFFER_SIZE, "hex"))
        {
            hex_mode = true;
            continue;
        }
        if (starts_with(req_buffer, BUFFER_SIZE, "char"))
        {
            hex_mode = false;
            continue;
        }
        result = send_message(sockfd, req_buffer, message_size);
        EXIT_IF(IS_ERROR(result), , "Error: Could not send message.\n");

        char *res_buffer = NULL;
        int res_buffer_size = 0;
        result = recv_message(sockfd, &res_buffer, &res_buffer_size);
        EXIT_IF(IS_ERROR(result), , "Error: Could not receive message.\n");

        char *res_str = NULL;
        if (hex_mode)
            result = buffer_to_hex_str(res_buffer, res_buffer_size, &res_str);
        else
            result = buffer_to_str(res_buffer, res_buffer_size, &res_str);
        free(res_buffer);
        EXIT_IF(IS_ERROR(result), , "Error: Could not receive message.\n");
        printf("%s\n", res_str);
        free(res_str);
    }

    close(sockfd);
    return 0;
}