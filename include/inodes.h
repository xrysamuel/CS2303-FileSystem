#ifndef INODES_H
#define INODES_H

#include "common.h"
#include "fsconfig.h"

/* This structure is similar to a clock, where
 * entry_1, entry_2, entry_3 are hours, minutes,
 * seconds. nth block -> visit path:
 * -1 -> 0 -1 0 0
 * 0 -> 0 0 0 0 BLOCK_START
 * 1 -> 0 1 0 0
 * 2 -> 0 2 0 0
 * ...
 * 11 -> 0 11 0 0
 * 12 -> 1 0 0 0 SBLOCK_START
 * 13 -> 1 1 0 0
 * 14 -> 1 2 0 0
 * ...
 * 75 -> 1 63 0 0   
 * 76 -> 2 0 0 0 DBLOCK_START
 * 77 -> 2 0 1 0
 * 78 -> 2 0 2 0
 * ...
 * 4171 -> 2 63 63 0
 * 4172 -> 3 0 0 0 TBLOCK_START
 * 4173 -> 3 0 0 1
 * 4174 -> 3 0 0 2
 * ...
 * 266315 -> 3 63 63 63
 */
struct visit_path_t
{
    enum
    {
        DIRECT_PATH = 0,
        SINGLE_PATH = 1,
        DOUBLE_PATH = 2,
        TRIPLE_PATH = 3,
    } visit_type;
    int entry_1;
    int entry_2;
    int entry_3;
};

enum op_t
{
    ALLOCATE_BLOCK_ID,  // allocate block id at the given entry
    DEALLOCATE_BLOCK_ID, // deallocate block id at the given entry
    GET_BLOCK_ID, // do nothing, simply get the block id at the given entry
};

void inodes_close();

void inodes_init(const char *server_ip, int port);

int inodes_format();

int create_inode(int *inode_id, u_int16_t mode, u_int16_t uid, u_int16_t gid);

int delete_inode(int inode_id);

int inode_file_resize(int inode_id, int size);

int inode_file_read(int inode_id, char *buffer, int start, int size);

int inode_file_write(int inode_id, const char *buffer, int start, int size);

int get_inode(int inode_id, struct inode_t* inode);

#endif