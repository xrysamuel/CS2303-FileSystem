#include "common.h"
#include "fs.h"
#include "error_type.h"
#include "server.h"
#include "buffer.h"

struct context_t contexts[MAX_CLIENTS];

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
    else if (starts_with(req_buffer, req_size, "e"))
    {
        return DEFAULT_ERROR;
    }
    else
    {
        return str_to_buffer("Error: Wrong format.", res_buffer, p_res_size, max_res_size);
    }
}

void handle_sigint(int sig)
{
    fs_close();
    exit(SUCCESS);
}

int main(int argc, char *argv[])
{
    EXIT_IF(argc != 4, , "Usage: %s <disk server address> <#disk port> <#fs port>\n", argv[0]);

    fs_init(argv[1], atoi(argv[2]));
    // fs_format();

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        contexts[i].cur_inode_id = 0;
        contexts[i].gid = 0;
        contexts[i].uid = 0;
    }

    signal(SIGINT, handle_sigint);

    simple_server(atoi(argv[3]), response);

    fs_close();

    return 0;
}