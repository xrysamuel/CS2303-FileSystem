#include "std.h"
#include "socket.h"
#include "buffer.h"
#include "error_type.h"
#include "client.h"

#define BUFFER_SIZE 1048576

bool hex_mode = false;

int get_request(char **req_buffer, int *req_size, int cycle)
{
    printf("> ");
    char *buffer = (char *)malloc(BUFFER_SIZE);
    RET_ERR_IF(buffer == NULL, , BAD_ALLOC_ERROR);

    fgets(buffer, BUFFER_SIZE, stdin);
    char *find = strchr(buffer, '\n');
    if (find)
        *find = '\0';
    
    if (starts_with(buffer, BUFFER_SIZE, "hex"))
        hex_mode = true;
    if (starts_with(buffer, BUFFER_SIZE, "char"))
        hex_mode = false;

    *req_buffer = buffer;
    *req_size = strlen(buffer);
    return *req_size;
}

int handle_response(const char *res_buffer, int res_size, int cycle)
{
    char *res_str = NULL;
    int result;
    if (hex_mode)
        result = buffer_to_hex_str(res_buffer, res_size, &res_str);
    else
        result = buffer_to_str(res_buffer, res_size, &res_str);
    RET_ERR_RESULT(result); 
    printf("%s\n", res_str);
    free(res_str);
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