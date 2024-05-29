#include "inodes.h"
#include "common.h"
#include "fsconfig.h"
#include "blocks.h"
#include "error_type.h"
#include <time.h>

void inodes_close()
{
    blocks_close();
}

void inodes_init(const char *server_ip, int port)
{
    blocks_init(server_ip, port);
}

int inodes_format()
{
    return blocks_format();
}

void nth_block_to_visit_path(int block, struct visit_path_t *p_visit_path)
{
    if (block < BLOCK_END)
    {
        p_visit_path->visit_type = DIRECT_PATH;
        p_visit_path->entry_1 = block;
        p_visit_path->entry_2 = 0;
        p_visit_path->entry_3 = 0;
    }
    else if (block < SBLOCK_END)
    {
        p_visit_path->visit_type = SINGLE_PATH;
        p_visit_path->entry_1 = (block - SBLOCK_START);
        p_visit_path->entry_2 = 0;
        p_visit_path->entry_3 = 0;
    }
    else if (block < DBLOCK_END)
    {
        p_visit_path->visit_type = DOUBLE_PATH;
        p_visit_path->entry_1 = (block - DBLOCK_START) / IB_N_ENTRIES;
        p_visit_path->entry_2 = (block - DBLOCK_START) % IB_N_ENTRIES;
        p_visit_path->entry_3 = 0;
    }
    else if (block < TBLOCK_END)
    {
        p_visit_path->visit_type = TRIPLE_PATH;
        p_visit_path->entry_1 = (block - TBLOCK_START) / (IB_N_ENTRIES * IB_N_ENTRIES);
        p_visit_path->entry_2 = ((block - TBLOCK_START) / IB_N_ENTRIES) % IB_N_ENTRIES;
        p_visit_path->entry_3 = (block - TBLOCK_START) % IB_N_ENTRIES;
    }
}

void visit_path_to_nth_block(struct visit_path_t visit_path, int *p_block)
{
    switch (visit_path.visit_type)
    {
    case DIRECT_PATH:
        *p_block = visit_path.entry_1;
        break;
    case SINGLE_PATH:
        *p_block = visit_path.entry_1 + SBLOCK_START;
        break;
    case DOUBLE_PATH:
        *p_block = visit_path.entry_1 * IB_N_ENTRIES + visit_path.entry_2 + DBLOCK_START;
        break;
    case TRIPLE_PATH:
        *p_block = visit_path.entry_1 * IB_N_ENTRIES * IB_N_ENTRIES + visit_path.entry_2 * IB_N_ENTRIES + visit_path.entry_3 + TBLOCK_START;
    }
}

int manipulate_entry_1_block_id(struct inode_t *p_inode, int *p_block_id, struct visit_path_t visit_path, enum op_t op)
{
    int result;
    struct indirect_block_t ib;
    // load entries
    switch (visit_path.visit_type)
    {
    case DIRECT_PATH:
        memcpy(&ib, &p_inode->block_ptr, 12 * sizeof(u_int32_t)); // unsafe
        break;
    case SINGLE_PATH:
        result = read_block(p_inode->sblock_ptr, (char *)&ib);
        break;
    case DOUBLE_PATH:
        result = read_block(p_inode->dblock_ptr, (char *)&ib);
        break;
    case TRIPLE_PATH:
        result = read_block(p_inode->tblock_ptr, (char *)&ib);
        break;
    }
    RET_ERR_RESULT(result);

    // manipulate entry
    switch (op)
    {
    case GET_BLOCK_ID:
        *p_block_id = ib.block_ptr[visit_path.entry_1];
        break;
    case DEALLOCATE_BLOCK_ID:
        *p_block_id = ib.block_ptr[visit_path.entry_1];
        result = deallocate_block(*p_block_id);
        RET_ERR_RESULT(result);
        break;
    case ALLOCATE_BLOCK_ID:
        result = allocate_block((int *)&ib.block_ptr[visit_path.entry_1]);
        RET_ERR_RESULT(result);
        *p_block_id = ib.block_ptr[visit_path.entry_1];
        break;
    }

    // save entries
    switch (visit_path.visit_type)
    {
    case DIRECT_PATH:
        memcpy(&p_inode->block_ptr, &ib, 12 * sizeof(u_int32_t)); // unsafe
        break;
    case SINGLE_PATH:
        result = write_block(p_inode->sblock_ptr, (char *)&ib);
        break;
    case DOUBLE_PATH:
        result = write_block(p_inode->dblock_ptr, (char *)&ib);
        break;
    case TRIPLE_PATH:
        result = write_block(p_inode->tblock_ptr, (char *)&ib);
        break;
    }
    RET_ERR_RESULT(result);
    return SUCCESS;
}

int manipulate_entry_2_block_id(struct inode_t *p_inode, int *p_block_id, struct visit_path_t visit_path, enum op_t op)
{
    RET_ERR_IF(visit_path.visit_type < 2, , INVALID_ARG_ERROR);

    // load entries
    int ib_id;
    int result = manipulate_entry_1_block_id(p_inode, &ib_id, visit_path, GET_BLOCK_ID);
    RET_ERR_RESULT(result);

    struct indirect_block_t ib;
    result = read_block(ib_id, (char *)&ib);
    RET_ERR_RESULT(result);

    // manipulate entry
    switch (op)
    {
    case GET_BLOCK_ID:
        *p_block_id = ib.block_ptr[visit_path.entry_2];
        break;
    case DEALLOCATE_BLOCK_ID:
        *p_block_id = ib.block_ptr[visit_path.entry_2];
        result = deallocate_block(*p_block_id);
        RET_ERR_RESULT(result);
        break;
    case ALLOCATE_BLOCK_ID:
        result = allocate_block((int *)&ib.block_ptr[visit_path.entry_2]);
        RET_ERR_RESULT(result);
        *p_block_id = ib.block_ptr[visit_path.entry_2];
        break;
    }

    // save entries
    result = write_block(ib_id, (char *)&ib);
    RET_ERR_RESULT(result);

    return SUCCESS;
}

int manipulate_entry_3_block_id(struct inode_t *p_inode, int *p_block_id, struct visit_path_t visit_path, enum op_t op)
{
    RET_ERR_IF(visit_path.visit_type < 3, , INVALID_ARG_ERROR);

    // load entries
    int ib_id;
    int result = manipulate_entry_2_block_id(p_inode, &ib_id, visit_path, GET_BLOCK_ID);
    RET_ERR_RESULT(result);

    struct indirect_block_t ib;
    result = read_block(ib_id, (char *)&ib);
    RET_ERR_RESULT(result);

    // manipulate entry
    switch (op)
    {
    case GET_BLOCK_ID:
        *p_block_id = ib.block_ptr[visit_path.entry_3];
        break;
    case DEALLOCATE_BLOCK_ID:
        *p_block_id = ib.block_ptr[visit_path.entry_3];
        result = deallocate_block(*p_block_id);
        RET_ERR_RESULT(result);
        break;
    case ALLOCATE_BLOCK_ID:
        result = allocate_block((int *)&ib.block_ptr[visit_path.entry_3]);
        RET_ERR_RESULT(result);
        *p_block_id = ib.block_ptr[visit_path.entry_3];
        break;
    }

    // save entries
    result = write_block(ib_id, (char *)&ib);
    RET_ERR_RESULT(result);

    return SUCCESS;
}

int visit_path_to_block_id(struct inode_t *p_inode, int *p_block_id, struct visit_path_t visit_path)
{
    int result;
    switch (visit_path.visit_type)
    {
    case DIRECT_PATH:
        result = manipulate_entry_1_block_id(p_inode, p_block_id, visit_path, GET_BLOCK_ID);
        break;
    case SINGLE_PATH:
        result = manipulate_entry_1_block_id(p_inode, p_block_id, visit_path, GET_BLOCK_ID);
        break;
    case DOUBLE_PATH:
        result = manipulate_entry_2_block_id(p_inode, p_block_id, visit_path, GET_BLOCK_ID);
        break;
    case TRIPLE_PATH:
        result = manipulate_entry_3_block_id(p_inode, p_block_id, visit_path, GET_BLOCK_ID);
        break;
    }
    RET_ERR_RESULT(result);
    return SUCCESS;
}

int create_inode(int *inode_id, u_int16_t mode, u_int16_t uid, u_int16_t gid)
{
    int result = allocate_inode(inode_id);
    RET_ERR_RESULT(result);

    struct inode_t inode;
    inode.mode = mode;
    inode.uid = uid;
    inode.gid = gid;
    inode.size = 0;
    time_t cur = time(NULL);
    inode.atime = cur;
    inode.ctime = cur;
    inode.mtime = cur;
    inode.dtime = 0;

    return write_inode(*inode_id, &inode);
}

int inode_file_resize(int inode_id, int size)
{
    struct inode_t inode;
    int result = read_inode(inode_id, &inode);
    RET_ERR_RESULT(result);

    int n_blocks = (size != 0) ? (size / BLOCK_SIZE + 1) : 0;
    int cur_n_blocks = (inode.size != 0) ? (inode.size / BLOCK_SIZE + 1) : 0;
    RET_ERR_IF(n_blocks >= MAX_SIZE || n_blocks < 0, , INVALID_ARG_ERROR);

    if (cur_n_blocks == n_blocks)
    {
        return SUCCESS;
    }

    // truncate
    if (cur_n_blocks > n_blocks)
    {
        struct visit_path_t cur_visit_path;
        struct visit_path_t prev_visit_path;
        int discard_block_id;
        nth_block_to_visit_path(cur_n_blocks, &prev_visit_path);
        for (int block = cur_n_blocks - 1; block >= n_blocks; block--)
        {
            nth_block_to_visit_path(block, &cur_visit_path);
            if (cur_visit_path.entry_3 != prev_visit_path.entry_3)
            {
                result = manipulate_entry_3_block_id(&inode, &discard_block_id, cur_visit_path, DEALLOCATE_BLOCK_ID);
                RET_ERR_RESULT(result);
            }
            if (cur_visit_path.entry_2 != prev_visit_path.entry_2)
            {
                result = manipulate_entry_2_block_id(&inode, &discard_block_id, cur_visit_path, DEALLOCATE_BLOCK_ID);
                RET_ERR_RESULT(result);
            }
            if (cur_visit_path.entry_1 != prev_visit_path.entry_1)
            {
                result = manipulate_entry_1_block_id(&inode, &discard_block_id, cur_visit_path, DEALLOCATE_BLOCK_ID);
                RET_ERR_RESULT(result);
            }
            if (cur_visit_path.visit_type != prev_visit_path.visit_type)
            {
                switch (cur_visit_path.visit_type)
                {
                case SINGLE_PATH:
                    result = deallocate_block(inode.sblock_ptr);
                    break;
                case DOUBLE_PATH:
                    result = deallocate_block(inode.dblock_ptr);
                    break;
                case TRIPLE_PATH:
                    result = deallocate_block(inode.tblock_ptr);
                    break;
                case DIRECT_PATH:
                }
                RET_ERR_RESULT(result);
            }
            prev_visit_path = cur_visit_path;
        }
    }
    // append
    else
    {
        struct visit_path_t cur_visit_path;
        struct visit_path_t prev_visit_path;
        int new_block_id;
        nth_block_to_visit_path(cur_n_blocks - 1, &prev_visit_path);
        for (int block = cur_n_blocks; block < n_blocks; block++)
        {

            nth_block_to_visit_path(block, &cur_visit_path);
            if (cur_visit_path.visit_type != prev_visit_path.visit_type)
            {
                switch (cur_visit_path.visit_type)
                {
                case SINGLE_PATH:
                    result = allocate_block((int *)&inode.sblock_ptr);
                    break;
                case DOUBLE_PATH:
                    result = allocate_block((int *)&inode.dblock_ptr);
                    break;
                case TRIPLE_PATH:
                    result = allocate_block((int *)&inode.tblock_ptr);
                    break;
                case DIRECT_PATH:
                }
                RET_ERR_RESULT(result);
            }
            if (cur_visit_path.entry_1 != prev_visit_path.entry_1)
            {
                result = manipulate_entry_1_block_id(&inode, &new_block_id, cur_visit_path, ALLOCATE_BLOCK_ID);
                RET_ERR_RESULT(result);
            }
            if (cur_visit_path.entry_2 != prev_visit_path.entry_2)
            {
                result = manipulate_entry_2_block_id(&inode, &new_block_id, cur_visit_path, ALLOCATE_BLOCK_ID);
                RET_ERR_RESULT(result);
            }
            if (cur_visit_path.entry_3 != prev_visit_path.entry_3)
            {
                result = manipulate_entry_3_block_id(&inode, &new_block_id, cur_visit_path, DEALLOCATE_BLOCK_ID);
                RET_ERR_RESULT(result);
            }
            prev_visit_path = cur_visit_path;
        }
    }

    inode.size = size;
    inode.atime = time(NULL);
    inode.mtime = time(NULL);
    result = write_inode(inode_id, &inode);
    RET_ERR_RESULT(result);

    return SUCCESS;
}

int inode_file_read(int inode_id, char *buffer, int start, int size)
{
    int result;

    struct inode_t inode;
    result = read_inode(inode_id, &inode);
    RET_ERR_RESULT(result);

    int block_buffer_id;
    char block_buffer[BLOCK_SIZE];

    struct visit_path_t visit_path;
    nth_block_to_visit_path(start / BLOCK_SIZE, &visit_path);
    result = visit_path_to_block_id(&inode, &block_buffer_id, visit_path);
    RET_ERR_RESULT(result);

    result = read_block(block_buffer_id, block_buffer);
    RET_ERR_RESULT(result);

    int block_id;
    for (int addr = start; addr < start + size; addr++)
    {
        int nth_block = addr / BLOCK_SIZE;
        struct visit_path_t visit_path;
        nth_block_to_visit_path(nth_block, &visit_path);
        result = visit_path_to_block_id(&inode, &block_id, visit_path);
        RET_ERR_RESULT(result);

        // The block_id to be accessed is not the block_id of the block stored
        // in the buffer.
        if (block_buffer_id != block_id)
        {
            result = read_block(block_id, block_buffer);
            RET_ERR_RESULT(result);
            block_buffer_id = block_id;
        }
        buffer[addr - start] = block_buffer[addr % BLOCK_SIZE];
    }

    inode.atime = time(NULL);
    result = write_inode(inode_id, &inode);
    RET_ERR_RESULT(result);

    return SUCCESS;
}

int inode_file_write(int inode_id, const char *buffer, int start, int size)
{
    int result;

    struct inode_t inode;
    result = read_inode(inode_id, &inode);
    RET_ERR_RESULT(result);

    int block_buffer_id;
    char block_buffer[BLOCK_SIZE];

    struct visit_path_t visit_path;
    nth_block_to_visit_path(start / BLOCK_SIZE, &visit_path);
    result = visit_path_to_block_id(&inode, &block_buffer_id, visit_path);
    RET_ERR_RESULT(result);

    int block_id;
    for (int addr = start; addr < start + size; addr++)
    {
        int nth_block = addr / BLOCK_SIZE;
        struct visit_path_t visit_path;
        nth_block_to_visit_path(nth_block, &visit_path);
        result = visit_path_to_block_id(&inode, &block_id, visit_path);
        RET_ERR_RESULT(result);

        // The block_id to be written is not the block_id of the block stored in
        // the buffer.
        if (block_buffer_id != block_id)
        {
            result = write_block(block_buffer_id, block_buffer);
            RET_ERR_RESULT(result);
            block_buffer_id = block_id;
        }
        block_buffer[addr % BLOCK_SIZE] = buffer[addr - start];
    }
    result = write_block(block_id, block_buffer);
    RET_ERR_RESULT(result);

    inode.atime = time(NULL);
    inode.mtime = time(NULL);
    result = write_inode(inode_id, &inode);
    RET_ERR_RESULT(result);

    return SUCCESS;
}

int delete_inode(int inode_id)
{
    int result = inode_file_resize(inode_id, 0);
    RET_ERR_RESULT(result);

    result = deallocate_inode(inode_id);
    RET_ERR_RESULT(result);

    return SUCCESS;
}

int get_inode(int inode_id, struct inode_t *inode)
{
    return read_inode(inode_id, inode);
}
