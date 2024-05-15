#include "std.h"
#include "socket.h"
#include "buffer.h"
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

int main(int argc, char *argv[])
{
    EXIT_IF(argc != 3, , "Usage: %s <server_ip> <port>\n", argv[0]);

    // simulate 8 clients
    fork();
    fork();
    fork();

    const char *server_ip = argv[1];
    int port = atoi(argv[2]);

    int result;
    int sockfd = create_socket();
    connect_to_server(sockfd, server_ip, port);

    srand(time(NULL) + getpid());

    char req_buffer[BUFFER_SIZE];
    int cycle = 0;
    while (true)
    {
        if (cycle == 0)
        {
            sprintf(req_buffer, "I");
        }
        else
        {
            if (cycle % 5 == 0)
            {
                sprintf(req_buffer, "R %d %d", randint(0, n_cylinders), randint(0, n_sectors));
            }
            else
            {
                int size = randint(10, 600); // size of write data
                sprintf(req_buffer, "W %d %d %d %s", randint(0, n_cylinders), randint(0, n_sectors), size, randstr(size));
            }
        }
        printf("> %s\n", req_buffer);
        result = send_message(sockfd, req_buffer, strlen(req_buffer));
        EXIT_IF(IS_ERROR(result), , "Error: Could not send message.\n");

        char *res_buffer = NULL;
        int res_buffer_size = 0;
        result = recv_message(sockfd, &res_buffer, &res_buffer_size);
        EXIT_IF(IS_ERROR(result), , "Error: Could not receive message.\n");

        char *res_str = NULL;
        result = buffer_to_str(res_buffer, res_buffer_size, &res_str);
        free(res_buffer);
        EXIT_IF(IS_ERROR(result), , "Error: Could not receive message.\n");
        if (cycle == 0)
        {
            sscanf(res_str, "%d %d", &n_cylinders, &n_sectors);
            printf("%s\n", res_str);
        }
        else
        {
            printf("%s\n", res_str);
        }
        free(res_str);
        cycle++;
        usleep(randint(100000, 1000000));
    }

    close(sockfd);
    return 0;
}