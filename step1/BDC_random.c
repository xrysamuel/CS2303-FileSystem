#include "std.h"
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

int get_request(char **req_buffer, int *req_size, int cycle)
{
    char *buffer = (char *)malloc(BUFFER_SIZE);
    RET_ERR_IF(buffer == NULL, , BAD_ALLOC_ERROR);

    if (cycle == 0)
    {
        sprintf(buffer, "I");
    }
    else
    {
        if (cycle % 5 == 0)
        {
            sprintf(buffer, "R %d %d", randint(0, n_cylinders), randint(0, n_sectors));
        }
        else
        {
            int size = randint(10, 600); // size of write data
            sprintf(buffer, "W %d %d %d %s", randint(0, n_cylinders), randint(0, n_sectors), size, randstr(size));
        }
    }
    printf("> %s\n", buffer);

    *req_buffer = buffer;
    *req_size = strlen(buffer);
    return *req_size;
}

int handle_response(const char *res_buffer, int res_size, int cycle)
{
    int result;
    char *res_str = NULL;
    result = buffer_to_str(res_buffer, res_size, &res_str);
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