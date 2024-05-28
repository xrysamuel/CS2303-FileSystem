#ifndef INODE_H
#define INODE_H

#include "std.h"

#define BLOCK_SIZE 256

/*
 *   Configuration:
 *
 *   Partition:
 *  | superblock | block bitmap | inode bitmap | inode table | data blocks |
 *        1           16384            16           32768      < 33554432
 * 
 *  ID / Bitmap Offset:
 *  - inode_id: 0 ~ 32767 = 16 * 256 * 8 - 1
 *  - block_id: 0 ~ 33554432 = 16384 * 256 * 8 - 1
 *  - runtime block_id limit: (n_blocks - DATA_BLOCKS_PTR)
 * 
 *  Hierarchy:
 *  - disk.c: Provides a basic abstraction layer for the read and write 
 *            interfaces in basic disk server, including caching mechanism.
 *  - blocks.c: Provides allocation and deallocation interfaces for blocks 
 *              and inodes, as well as read and write interfaces for blocks.
 */

#define SUPERBLOCK_PTR 0
#define BLOCK_BITMAP_PTR 1
#define INODE_BITMAP_PTR 16385
#define INODE_TABLE_PTR 16401
#define DATA_BLOCKS_PTR 49169

#define MIN_N_BLOCKS 81937
#define MAX_N_BLOCKS 33603601

#define BLOCK_BITMAP_END INODE_BITMAP_PTR
#define INODE_BITMAP_END INODE_TABLE_PTR
#define INODE_TABLE_END DATA_BLOCKS_PTR

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

#define IB_N_ENTRIES 64

// Indirect Block
struct indirect_block_t
{
    u_int32_t block_ptr[IB_N_ENTRIES];
};

#define BLOCK_START 0
#define BLOCK_END 12
#define SBLOCK_START 12
#define SBLOCK_END 76
#define DBLOCK_START 76
#define DBLOCK_END 4172
#define TBLOCK_START 4172
#define TBLOCK_END 266316

#define MAX_SIZE 68176869

#define MODE_UR  0x0000000000000001 // owner read
#define MODE_UW  0x0000000000000010 // owner write
#define MODE_UX  0x0000000000000100 // owner execute
#define MODE_GR  0x0000000000001000 // group read
#define MODE_GW  0x0000000000010000 // group write
#define MODE_GX  0x0000000000100000 // group execute
#define MODE_OR  0x0000000001000000 // others read
#define MODE_OW  0x0000000010000000 // others write
#define MODE_OX  0x0000000100000000 // others execute
#define MODE_DIR 0x0000001000000000 // is directory
#define IS_MODE(mode, value) (((mode) & (value)) != 0)

static_assert(sizeof(struct superblock_t) == BLOCK_SIZE);
static_assert(sizeof(struct inode_t) == BLOCK_SIZE);
static_assert(sizeof(struct indirect_block_t) == BLOCK_SIZE);

#pragma pack()

#endif