#ifndef INODE_H
#define INODE_H

#include "std.h"

#define BLOCK_SIZE 256

/*
 *   
 *     | superblock | block bitmap | inode bitmap | inode table | data blocks |
 *           1           16384            16           32768      < 33554432
 */

#define SUPERBLOCK_PTR 0
#define BLOCK_BITMAP_PTR 1
#define INODE_BITMAP_PTR 16385
#define INODE_TABLE_PTR 16401
#define DATA_BLOCKS_PTR 49169

#define MIN_N_BLOCKS 81937
#define MAX_N_BLOCKS 33603601

#pragma pack(1)
struct superblock_t
{
    u_int32_t block_size;       // fixed as 256
    u_int32_t n_free_inodes;    // unallocated inode
    u_int32_t n_free_blocks;    // unallocated data blocks
    u_int32_t block_bitmap_ptr; // data block bitmap pointer, fixed as 1
    u_int32_t inode_bitmap_ptr; // inode block bitmap pointer, fixed as 16385
    u_int32_t inode_table_ptr;  // inode table pointer, fixed as 16401
    u_int32_t data_blocks_ptr;  // data blocks pointer, fixed as 49169
    char formatted;         // whether this partition has been formatted
    char reserved[227];
};

struct inode_t
{
    u_int16_t mode;          // type and permission
    u_int16_t uid;           // user id
    u_int32_t size;          // size in bytes, max size
    u_int32_t atime;         // last access time (in POSIX time)
    u_int32_t ctime;         // creation time (in POSIX time)
    u_int32_t mtime;         // last modification time (in POSIX time)
    u_int32_t dtime;         // deletion time (in POSIX time)
    u_int32_t block_ptr[12]; // direct block pointer
    u_int32_t sblock_ptr;    // singly indirect block pointer
    u_int32_t dblock_ptr;    // doubly indirect block pointer
    u_int32_t tblock_ptr;    // triply indirect block pointer
    u_int16_t gid;           // group id
    char reserved[170];
};

struct indirect_block_t
{
    u_int32_t block_addr[64];
};

struct bitmap_block_t
{
    char bytes[256];
};

static_assert(sizeof(struct superblock_t) == BLOCK_SIZE);
static_assert(sizeof(struct inode_t) == BLOCK_SIZE);
static_assert(sizeof(struct bitmap_block_t) == BLOCK_SIZE);
static_assert(sizeof(struct indirect_block_t) == BLOCK_SIZE);

#pragma pack()
#endif