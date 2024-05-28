#include "socket.h"
#include "error_type.h"
#include "common.h"

// Create a socket and return the file descriptor of the new socket.
int create_socket()
{
    // AF_INET - address domain
    // SOCK_STREAM - stream socket
    // 0 - appropriate protocol (TCP)
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    EXIT_IF(sockfd < 0, , "Error: Could not create socket.\n");

    signal(SIGPIPE, SIG_IGN);
    return sockfd;
}

// Bind the port to the socket file descriptor.
void bind_socket(int sockfd, uint16_t port)
{
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    // INADDR_ANY - get IP address of the host automatically
    serv_addr.sin_port = htons(port);
    int result = bind(sockfd, ((struct sockaddr *)(&serv_addr)), sizeof(serv_addr));
    EXIT_IF(result < 0, close(sockfd), "Error: Could not bind socket.\n");
}

// Listen on the provided socket file descriptor.
void listen_socket(int sockfd)
{
    int result = listen(sockfd, 5);
    EXIT_IF(result < 0, close(sockfd), "Error: Could not listen socket.\n");
}

// Wait for client connection on the socket file descriptor.
int wait_for_client(int sockfd)
{
    int client_sockfd;
    struct sockaddr_in client_addr;
    unsigned len = sizeof(client_addr);
    client_sockfd = accept(sockfd, ((struct sockaddr *)(&client_addr)), &len);
    EXIT_IF(client_sockfd < 0, close(sockfd), "Error: Could not bind socket.\n");
    return client_sockfd;
}

// Connects to a server using the provided socket file descriptor, IP address, and port number.
void connect_to_server(int sockfd, const char *ip, uint16_t port)
{
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    int result = inet_pton(AF_INET, ip, &server_addr.sin_addr.s_addr);
    EXIT_IF(result <= 0, close(sockfd), "Error: Invalid address.\n");

    result = connect(sockfd, ((struct sockaddr *)(&server_addr)), sizeof(server_addr));
    EXIT_IF(result < 0, close(sockfd), "Error: Connection failed.\n");
}

// Reads data from the socket and stores it in the provided buffer and its size.
// Returns the number of bytes read on success.
int readn(int sockfd, char *buffer, int size)
{
    int remain = size;
    char *p = buffer;
    while (remain > 0)
    {
        int n_read = read(sockfd, p, remain);
        if (n_read > 0)
        {
            p += n_read;
            remain -= n_read;
        }
        else
            RET_ERR_IF(n_read <= 0, , READ_ERROR);
    }
    return size;
}

// Writes data from the provided buffer to the socket.
// Returns the number of bytes written on success.
int writen(int sockfd, const char *buffer, int size)
{
    int remain = size;

    while (remain > 0)
    {
        int n_write = write(sockfd, buffer, remain);
        if (n_write > 0)
        {
            buffer += n_write;
            remain -= n_write;
        }
        else
            RET_ERR_IF(n_write <= 0, , WRITE_ERROR);
    }
    return size;
}

// Sends a message to the server using the provided socket file descriptor, buffer, and size.
// Returns the size of the message on success.
int send_message(int sockfd, const char *buffer, int size)
{
    RET_ERR_IF(sockfd < 0 || buffer == NULL || size < 0, , INVALID_ARG_ERROR);

    char *internal_buffer = (char *)malloc(size + 4);
    EXIT_IF(internal_buffer == NULL, close(sockfd), "Error: Bad alloc.\n");

    int big_len = htonl(size);
    memcpy(internal_buffer, &big_len, 4);
    memcpy(internal_buffer + 4, buffer, size);
    int result = writen(sockfd, internal_buffer, size + 4);
    free(internal_buffer);
    return result;
}

// Receives a message from the server using the provided socket file descriptor.
// Returns the size of message on success.
int recv_message(int sockfd, char *buffer, int *p_size, int max_size)
{
    RET_ERR_IF(sockfd < 0, , INVALID_ARG_ERROR);

    int size = 0;
    readn(sockfd, (char *)&size, 4);
    size = ntohl(size);
    RET_ERR_IF(size < 0, , READ_ERROR);
    RET_ERR_IF(size > max_size, , BUFFER_OVERFLOW);

    int result = readn(sockfd, buffer, size);
    RET_ERR_IF(result != size, , READ_ERROR);

    *p_size = size;
    return result;
}

void sigpipe_handler(int sig)
{
    pthread_exit(NULL);
}