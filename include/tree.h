#ifndef TREE_H
#define TREE_H

#define MAX_NAME_LEN 256 // including \0

#include "common.h"

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

struct dir_entry_t
{
    char name[MAX_NAME_LEN];
    int inode_id;
};

#define DIR_ENTRY_SIZE sizeof(struct dir_entry_t)

#endif