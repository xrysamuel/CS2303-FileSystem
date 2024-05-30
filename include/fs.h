#ifndef FS_H
#define FS_H

#define MAX_NAME_LEN 252 // including \0
#define MAX_USERNAME_LEN 252
#define MAX_PASSWORD_LEN 248

#include "common.h"

void fs_close();

void fs_init(const char *server_ip, int port);

int fs_format();

/*
 * Directory:
 *
 * inode_id = 0: root directory
 * uid = 0: root user
 * gid = 0: root user group
 */

struct context_t
{
    u_int32_t uid;
    u_int32_t gid;
    int cur_inode_id;
};

#define ROOT_INODE_ID 0
#define ROOT_UID 0
#define ROOT_GID 0
#define PASSWD_INODE_ID 1

struct dir_entry_t
{
    char name[MAX_NAME_LEN];
    int inode_id;
};

struct acc_entry_t
{
    char name[MAX_USERNAME_LEN];
    char password[MAX_PASSWORD_LEN];
    int uid; // < 0 means invalid
    int gid;
};

#define DIR_ENTRY_SIZE sizeof(struct dir_entry_t)
#define ACC_ENTRY_SIZE sizeof(struct acc_entry_t)

enum auth_t
{
    READ_AUTH,
    WRITE_AUTH,
    EXE_AUTH,
    NO_AUTH
};

// FS operation

struct response_arg_t
{
    // context
    struct context_t *p_context;

    // response
    char *res_buffer;
    int *p_res_size;
    int max_size;

    // request
    const char *req_buffer;
    int req_size;
};

typedef int (*fs_op_t)(struct response_arg_t arg, int *p_n_entries, struct dir_entry_t **entries);

int fs_operation_wrapper(fs_op_t fs_op, struct response_arg_t arg, enum auth_t auth);

int list(struct response_arg_t arg, int *p_n_entries, struct dir_entry_t **entries);

int make_file(struct response_arg_t arg, int *p_n_entries, struct dir_entry_t **p_entries);

int remove_file(struct response_arg_t arg, int *p_n_entries, struct dir_entry_t **p_entries);

int make_dir(struct response_arg_t arg, int *p_n_entries, struct dir_entry_t **p_entries);

int remove_dir(struct response_arg_t arg, int *p_n_entries, struct dir_entry_t **p_entries);

int change_dir(struct response_arg_t arg, int *p_n_entries, struct dir_entry_t **p_entries);

int catch_file(struct response_arg_t arg, int *p_n_entries, struct dir_entry_t **p_entries);

int write_file(struct response_arg_t arg, int *p_n_entries, struct dir_entry_t **p_entries);

int insert_file(struct response_arg_t arg, int *p_n_entries, struct dir_entry_t **p_entries);

int delete_in_file(struct response_arg_t arg, int *p_n_entries, struct dir_entry_t **p_entries);

int change_account(struct response_arg_t arg, int *p_n_entries, struct dir_entry_t **p_entries);

int remove_account(struct response_arg_t arg, int *p_n_entries, struct dir_entry_t **p_entries);

int chmod_file(struct response_arg_t arg, int *p_n_entries, struct dir_entry_t **p_entries);

#define MATCHES_QUERY(key, query, query_size) (strncmp((key), (query), (query_size)) == 0 && strlen(key) == (query_size))

#endif