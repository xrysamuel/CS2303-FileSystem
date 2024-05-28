#include "definition.h"
#include "std.h"
#include "disk.h"
#include "error_type.h"

/*
 * buffer
 */

static int n_blocks;
static struct superblock_t superblock;

/*
 * bitmap history
 */

int least_block_bitmap_block = 0;
int least_inode_bitmap_block = 0;

/*
 * init & close
 */

void blocks_close()
{
    int result = disk_write((char *)&superblock, SUPERBLOCK_PTR);
    EXIT_IF(IS_ERROR(result), disk_close(), "FATAL: could not write superblock.\n");
    disk_close();
}

int blocks_format()
{
    superblock.block_size = BLOCK_SIZE;
    superblock.n_free_inodes = INODE_TABLE_END - INODE_TABLE_PTR;
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

    for (int i = BLOCK_BITMAP_PTR; i < INODE_BITMAP_END; i++)
    {
        result = disk_write(zeros, i);
        RET_ERR_RESULT(result); 
    }

    superblock.formatted = true;
    result = disk_write((char *)&superblock, SUPERBLOCK_PTR);
    RET_ERR_RESULT(result); 

    least_block_bitmap_block = 0;
    least_inode_bitmap_block = 0;

    return SUCCESS;
}

void blocks_init(const char *server_ip, int port)
{
    disk_init(server_ip, port);

    get_n_blocks(&n_blocks);
    EXIT_IF(n_blocks > MAX_N_BLOCKS || n_blocks < MIN_N_BLOCKS, blocks_close(), "Error: Invalid disk size.\n");

    int result = disk_read((char *)&superblock, SUPERBLOCK_PTR);
    EXIT_IF(IS_ERROR(result), blocks_close(), "Error: Could not read super block.\n");

    if (!superblock.formatted)
    {
        result = blocks_format();
        EXIT_IF(IS_ERROR(result), blocks_close(), "Error: Failed to format the disk.\n");
    }

    least_block_bitmap_block = 0;
    least_inode_bitmap_block = 0;
}

/*
 * bitmap
 */

int bit_set(int start_block, int offset)
{
    char buffer[BLOCK_SIZE];
    int block = start_block + (offset / (BLOCK_SIZE * 8));
    int byte_offset = (offset % (BLOCK_SIZE * 8)) / 8;
    int bit_offset = (offset % (BLOCK_SIZE * 8)) % 8;

    int result = disk_read(buffer, block);
    RET_ERR_RESULT(result); 

    buffer[byte_offset] |= (1 << bit_offset);

    result = disk_write(buffer, block);
    RET_ERR_RESULT(result); 

    return SUCCESS;
}

int bit_clear(int start_block, int offset)
{
    char buffer[BLOCK_SIZE];
    int block = start_block + (offset / (BLOCK_SIZE * 8));
    int byte_offset = (offset % (BLOCK_SIZE * 8)) / 8;
    int bit_offset = (offset % (BLOCK_SIZE * 8)) % 8;

    int result = disk_read(buffer, block);
    RET_ERR_RESULT(result); 

    buffer[byte_offset] &= ~(1 << bit_offset);

    result = disk_write(buffer, block);
    RET_ERR_RESULT(result); 
    return 0;
}

int find_first_zero_bit(int start_block, int *p_offset, int search_start_block, int search_end_block)
{
    char buffer[BLOCK_SIZE];
    int block = search_start_block;

    while (block < search_end_block)
    {
        int result = disk_read(buffer, block);
        RET_ERR_RESULT(result); 

        for (int offset = 0; offset < BLOCK_SIZE * 8; offset++)
        {
            int byte_offset = offset / 8;
            int bit_offset = offset % 8;

            if ((buffer[byte_offset] & (1 << bit_offset)) == 0)
            {
                *p_offset = ((block - start_block) * (BLOCK_SIZE * 8)) + (byte_offset * 8) + bit_offset;
                return 0;
            }
        }

        block++;
    }
    return DEFAULT_ERROR;
}

/*
 * inode
 */

int deallocate_inode(int inode_id)
{
    RET_ERR_IF(inode_id < 0, , INVALID_ARG_ERROR);
    RET_ERR_IF(inode_id >= INODE_TABLE_END - INODE_TABLE_PTR, , INVALID_ARG_ERROR);

    int result = bit_clear(INODE_BITMAP_PTR, inode_id);
    RET_ERR_RESULT(result); 
    superblock.n_free_inodes++;
    least_inode_bitmap_block = INODE_BITMAP_PTR + inode_id / (BLOCK_SIZE * 8);
    return SUCCESS;
}

int allocate_inode(int *inode_id)
{
    RET_ERR_IF(superblock.n_free_inodes <= 0, , DISK_FULL_ERROR);
    int inode_bitmap_offset;
    int result = find_first_zero_bit(INODE_BITMAP_PTR, &inode_bitmap_offset, least_inode_bitmap_block, INODE_BITMAP_END);
    RET_ERR_RESULT(result); 

    least_inode_bitmap_block = INODE_BITMAP_PTR + inode_bitmap_offset / (BLOCK_SIZE * 8);

    result = bit_set(INODE_BITMAP_PTR, inode_bitmap_offset);
    RET_ERR_RESULT(result); 
    *inode_id = inode_bitmap_offset;
    superblock.n_free_inodes--;
    return SUCCESS;
}

int read_inode(int inode_id, struct inode_t *p_inode)
{
    RET_ERR_IF(inode_id < 0, , INVALID_ARG_ERROR);
    RET_ERR_IF(inode_id >= INODE_TABLE_END - INODE_TABLE_PTR, , INVALID_ARG_ERROR);

    return disk_read((char *)p_inode, INODE_TABLE_PTR + inode_id);
}

int write_inode(int inode_id, const struct inode_t* p_inode)
{
    RET_ERR_IF(inode_id < 0, , INVALID_ARG_ERROR);
    RET_ERR_IF(inode_id >= INODE_TABLE_END - INODE_TABLE_PTR, , INVALID_ARG_ERROR);

    return disk_write((char *)p_inode, INODE_TABLE_PTR + inode_id);
}

/*
 * data block
 */

int deallocate_block(int block_id)
{
    RET_ERR_IF(block_id < 0, , INVALID_ARG_ERROR);
    RET_ERR_IF(block_id >= n_blocks - DATA_BLOCKS_PTR, , INVALID_ARG_ERROR);

    int result = bit_clear(BLOCK_BITMAP_PTR, block_id);
    RET_ERR_RESULT(result); 
    superblock.n_free_blocks++;
    least_block_bitmap_block = BLOCK_BITMAP_PTR + block_id / (BLOCK_SIZE * 8);
    return SUCCESS;
}

int allocate_block(int *block_id)
{
    RET_ERR_IF(superblock.n_free_blocks <= 0, , DISK_FULL_ERROR);
    int block_bitmap_offset;
    int result = find_first_zero_bit(BLOCK_BITMAP_PTR, &block_bitmap_offset, least_block_bitmap_block, BLOCK_BITMAP_END);
    RET_ERR_RESULT(result); 

    least_block_bitmap_block = BLOCK_BITMAP_PTR + block_bitmap_offset / (BLOCK_SIZE * 8);

    result = bit_set(BLOCK_BITMAP_PTR, block_bitmap_offset);
    RET_ERR_RESULT(result); 
    *block_id = block_bitmap_offset;
    superblock.n_free_blocks--;
    return SUCCESS;
}

int read_block(int block_id, char block[BLOCK_SIZE])
{
    RET_ERR_IF(block_id < 0, , INVALID_ARG_ERROR);
    RET_ERR_IF(block_id >= n_blocks - DATA_BLOCKS_PTR, , INVALID_ARG_ERROR);

    return disk_read(block, DATA_BLOCKS_PTR + block_id);
}

int write_block(int block_id, const char block[BLOCK_SIZE])
{
    RET_ERR_IF(block_id < 0, , INVALID_ARG_ERROR);
    RET_ERR_IF(block_id >= n_blocks - DATA_BLOCKS_PTR, , INVALID_ARG_ERROR);

    return disk_write(block, DATA_BLOCKS_PTR + block_id);
}