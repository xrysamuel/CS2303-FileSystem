#include "common.h"
#include "socket.h"
#include "buffer.h"
#include "error_type.h"
#include "client.h"

bool hex_mode = false;

int get_request(char *req_buffer, int *p_req_size, int max_req_size, int cycle)
{
    printf("> ");

    fgets(req_buffer, max_req_size, stdin);
    char *find = strchr(req_buffer, '\n');
    if (find)
        *find = '\0';

    if (starts_with(req_buffer, max_req_size, "hex"))
        hex_mode = true;
    if (starts_with(req_buffer, max_req_size, "char"))
        hex_mode = false;

    *p_req_size = strlen(req_buffer);
    return *p_req_size;
}

int handle_response(const char *res_buffer, int res_size, int cycle)
{
    char res_str[DEFAULT_BUFFER_CAPACITY];
    int result;
    if (hex_mode)
        result = buffer_to_hex_str(res_buffer, res_size, res_str, DEFAULT_BUFFER_CAPACITY);
    else
        result = buffer_to_str(res_buffer, res_size, res_str, DEFAULT_BUFFER_CAPACITY);
    RET_ERR_RESULT(result);
    printf("%s\n", res_str);
    return res_size;
}

int main(int argc, char *argv[])
{

    EXIT_IF(argc != 3, , "Usage: %s <server_ip> <port>\n", argv[0]);

    const char *server_ip = argv[1];
    int port = atoi(argv[2]);

    simple_client(server_ip, port, get_request, handle_response);

    return 0;
}