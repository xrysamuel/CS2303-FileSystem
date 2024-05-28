#include "common.h"
#include "socket.h"
#include "buffer.h"
#include "client.h"
#include "error_type.h"

#define BUFFER_SIZE 1048576

int n_cylinders = 512;
int n_sectors = 16;

int randint(int a, int b)
{
    return (rand() % (b - a) + a);
}

char *randstr(int size)
{
    static char random_string[BUFFER_SIZE];

    for (int i = 0; i < size; i++)
    {
        int index = randint(0x20, 0x7F);
        random_string[i] = (char)index;
    }

    random_string[size] = '\0';
    return random_string;
}

int get_request(char *req_buffer, int *p_req_size, int max_req_size, int cycle)
{
    int n_print;
    if (cycle == 0)
    {
        n_print = snprintf(req_buffer, max_req_size, "I");
        RET_ERR_IF(n_print >= max_req_size, , BUFFER_OVERFLOW);
    }
    else
    {
        if (cycle % 5 == 0)
        {
            n_print = snprintf(req_buffer, max_req_size, "R %d %d", randint(0, n_cylinders), randint(0, n_sectors));
            RET_ERR_IF(n_print >= max_req_size, , BUFFER_OVERFLOW);
        }
        else
        {
            int size = randint(10, 600); // size of write data
            n_print = snprintf(req_buffer, max_req_size, "W %d %d %d %s", randint(0, n_cylinders), randint(0, n_sectors), size, randstr(size));
            RET_ERR_IF(n_print >= max_req_size, , BUFFER_OVERFLOW);
        }
    }
    printf("> %s\n", req_buffer);

    *p_req_size = strlen(req_buffer);
    return *p_req_size;
}

int handle_response(const char *res_buffer, int res_size, int cycle)
{
    int result;
    char res_str[DEFAULT_BUFFER_CAPACITY];
    result = buffer_to_str(res_buffer, res_size, res_str, DEFAULT_BUFFER_CAPACITY);
    RET_ERR_RESULT(result);

    if (cycle == 0)
    {
        sscanf(res_str, "%d %d", &n_cylinders, &n_sectors);
        printf("%s\n", res_str);
    }
    else
    {
        printf("%s\n", res_str);
    }
    usleep(randint(100000, 1000000));
    return res_size;
}

int main(int argc, char *argv[])
{
    EXIT_IF(argc != 3, , "Usage: %s <server_ip> <port>\n", argv[0]);

    // simulate 8 clients
    fork();
    fork();
    fork();

    const char *server_ip = argv[1];
    int port = atoi(argv[2]);
    srand(time(NULL) + getpid());

    simple_client(server_ip, port, get_request, handle_response);

    return 0;
}