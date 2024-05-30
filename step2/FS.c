#include "common.h"
#include "fs.h"
#include "error_type.h"
#include "server.h"
#include "buffer.h"

struct context_t contexts[MAX_CLIENTS]; // contexts[0] used as internal context
sem_t response_mutex;

int response(int sockfd, const char *req_buffer, int req_size, char *res_buffer, int *p_res_size, int max_res_size)
{
    if (starts_with(req_buffer, req_size, "f"))
    {
        return fs_format();
    }
    else if (starts_with(req_buffer, req_size, "mk "))
    {
        struct response_arg_t arg = {&contexts[sockfd], res_buffer, p_res_size, max_res_size, req_buffer + 3, req_size - 3};
        return fs_operation_wrapper(make_file, arg, WRITE_AUTH);
    }
    else if (starts_with(req_buffer, req_size, "mkdir "))
    {
        struct response_arg_t arg = {&contexts[sockfd], res_buffer, p_res_size, max_res_size, req_buffer + 6, req_size - 6};
        return fs_operation_wrapper(make_dir, arg, WRITE_AUTH);
    }
    else if (starts_with(req_buffer, req_size, "rm "))
    {
        struct response_arg_t arg = {&contexts[sockfd], res_buffer, p_res_size, max_res_size, req_buffer + 3, req_size - 3};
        return fs_operation_wrapper(remove_file, arg, WRITE_AUTH);
    }
    else if (starts_with(req_buffer, req_size, "cd "))
    {
        struct response_arg_t arg = {&contexts[sockfd], res_buffer, p_res_size, max_res_size, req_buffer + 3, req_size - 3};
        return fs_operation_wrapper(change_dir, arg, NO_AUTH);
    }
    else if (starts_with(req_buffer, req_size, "rmdir "))
    {
        struct response_arg_t arg = {&contexts[sockfd], res_buffer, p_res_size, max_res_size, req_buffer + 6, req_size - 6};
        return fs_operation_wrapper(remove_dir, arg, WRITE_AUTH);
    }
    else if (starts_with(req_buffer, req_size, "ls"))
    {
        struct response_arg_t arg = {&contexts[sockfd], res_buffer, p_res_size, max_res_size, req_buffer + 3, req_size - 3};
        return fs_operation_wrapper(list, arg, NO_AUTH);
    }
    else if (starts_with(req_buffer, req_size, "cat "))
    {
        struct response_arg_t arg = {&contexts[sockfd], res_buffer, p_res_size, max_res_size, req_buffer + 4, req_size - 4};
        return fs_operation_wrapper(catch_file, arg, WRITE_AUTH);
    }
    else if (starts_with(req_buffer, req_size, "w "))
    {
        struct response_arg_t arg = {&contexts[sockfd], res_buffer, p_res_size, max_res_size, req_buffer + 2, req_size - 2};
        return fs_operation_wrapper(write_file, arg, WRITE_AUTH);
    }
    else if (starts_with(req_buffer, req_size, "i "))
    {
        struct response_arg_t arg = {&contexts[sockfd], res_buffer, p_res_size, max_res_size, req_buffer + 2, req_size - 2};
        return fs_operation_wrapper(insert_file, arg, WRITE_AUTH);
    }
    else if (starts_with(req_buffer, req_size, "d "))
    {
        struct response_arg_t arg = {&contexts[sockfd], res_buffer, p_res_size, max_res_size, req_buffer + 2, req_size - 2};
        return fs_operation_wrapper(delete_in_file, arg, WRITE_AUTH);
    }
    else if (starts_with(req_buffer, req_size, "cacc "))
    {
        struct response_arg_t arg = {&contexts[sockfd], res_buffer, p_res_size, max_res_size, req_buffer + 5, req_size - 5};
        return fs_operation_wrapper(change_account, arg, WRITE_AUTH);
    }
    else if (starts_with(req_buffer, req_size, "rmacc "))
    {
        struct response_arg_t arg = {&contexts[sockfd], res_buffer, p_res_size, max_res_size, req_buffer + 6, req_size - 6};
        return fs_operation_wrapper(remove_account, arg, WRITE_AUTH);
    }
    else if (starts_with(req_buffer, req_size, "chmod "))
    {
        struct response_arg_t arg = {&contexts[sockfd], res_buffer, p_res_size, max_res_size, req_buffer + 6, req_size - 6};
        return fs_operation_wrapper(chmod_file, arg, WRITE_AUTH);
    }
    else if (starts_with(req_buffer, req_size, "e"))
    {
        return DEFAULT_ERROR;
    }
    else
    {
        return str_to_buffer("Error: Wrong format.", res_buffer, p_res_size, max_res_size);
    }
}

int response_with_mutex(int sockfd, const char *req_buffer, int req_size, char *res_buffer, int *p_res_size, int max_res_size)
{
    sem_wait(&response_mutex);
    int result = response(sockfd, req_buffer, req_size, res_buffer, p_res_size, max_res_size);
    sem_post(&response_mutex);
    return result;
}

void handle_sigint(int sig)
{
    sem_destroy(&response_mutex);
    fs_close();
    exit(SUCCESS);
}

int main(int argc, char *argv[])
{
    EXIT_IF(argc != 4, , "Usage: %s <disk server address> <#disk port> <#fs port>\n", argv[0]);

    fs_init(argv[1], atoi(argv[2]));
    sem_init(&response_mutex, 0, 1);
    // fs_format();

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        contexts[i].cur_inode_id = 0;
        contexts[i].gid = 0;
        contexts[i].uid = 0;
    }

    signal(SIGINT, handle_sigint);

    simple_server(atoi(argv[3]), response_with_mutex);

    sem_destroy(&response_mutex);
    fs_close();

    return 0;
}