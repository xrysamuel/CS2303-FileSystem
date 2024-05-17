#include "definition.h"
#include "std.h"
#include "disk.h"
#include "error_type.h"

// buffer
static int n_blocks;
static struct superblock_t superblock;
static char cache[256];

void files_close()
{
    disk_close();
}

int files_format()
{
    superblock.block_size = BLOCK_SIZE;
    superblock.n_free_inodes = DATA_BLOCKS_PTR - INODE_TABLE_PTR;
    superblock.n_free_blocks = n_blocks - DATA_BLOCKS_PTR;
    superblock.block_bitmap_ptr = BLOCK_BITMAP_PTR;
    superblock.inode_bitmap_ptr = INODE_BITMAP_PTR;
    superblock.inode_table_ptr = INODE_TABLE_PTR;
    superblock.data_blocks_ptr = DATA_BLOCKS_PTR;
    
    int result;
    char zeros[256];
    for (int i = 0; i < 256; i++)
    {
        zeros[i] = 0;
    }

    for (int i = BLOCK_BITMAP_PTR; i < INODE_TABLE_PTR; i++)
    {
        result = disk_write_from(zeros, i);
        RET_ERR_IF(IS_ERROR(result), , result);
    }

    superblock.formatted = true;
    result = disk_write_from((char *) &superblock, SUPERBLOCK_PTR);
    RET_ERR_IF(IS_ERROR(result), , result);
    return 0;
}   


void files_init(const char *server_ip, int port)
{
    disk_init(server_ip, port);

    get_n_blocks(&n_blocks);
    EXIT_IF(n_blocks > MAX_N_BLOCKS || n_blocks < MIN_N_BLOCKS, files_close(), "Error: Invalid disk size.\n");

    int result = disk_read_to((char *) &superblock, SUPERBLOCK_PTR);
    EXIT_IF(IS_ERROR(result), files_close(), "Error: Could not read super block.\n");

    if(!superblock.formatted)
    {
        result = files_format();
        EXIT_IF(IS_ERROR(result), files_close(), "Error: Failed to format the disk.\n");
    }

}