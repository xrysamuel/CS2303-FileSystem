#include "tree.h"
#include "error_type.h"
#include "fsconfig.h"
#include "inodes.h"
#include "buffer.h"

int cur_inode_id;

void tree_close()
{
    inodes_close();
}

void tree_init(const char *server_ip, int port)
{
    inodes_init(server_ip, port);
    cur_inode_id = ROOT_INODE_ID;
}

int tree_format()
{
    int result = inodes_format();
    RET_ERR_RESULT(result);

    int inode_id;
    // root directory
    result = create_inode(&inode_id, MODE_UR | MODE_UW | MODE_UX | MODE_GR | MODE_GW | MODE_GX | MODE_OR | MODE_OW | MODE_OX | MODE_DIR, ROOT_UID, ROOT_GID);
    RET_ERR_RESULT(result);
    RET_ERR_IF(inode_id != ROOT_INODE_ID, , DEFAULT_ERROR);

    return SUCCESS;
}


int list(struct context_t *p_context, char *res_buffer, char *p_size, int max_size)
{
    int result;
    struct inode_t inode;
    result = get_inode(p_context->cur_inode_id, &inode);
    RET_ERR_IF(IS_ERROR(result), p_context->cur_inode_id = 0, str_to_buffer("FATAL: Could not access disk.", res_buffer, p_size, max_size));

    int n_entries = inode.size / DIR_ENTRY_SIZE;
    struct dir_entry_t *entries = (struct dir_entry_t *)malloc(inode.size);
    RET_ERR_IF(entries == NULL, , BAD_ALLOC_ERROR);
    result = inode_file_read(p_context->cur_inode_id, (char *)entries, 0, inode.size);
    RET_ERR_IF(IS_ERROR(result), free(entries), str_to_buffer("FATAL: Could not access disk.", res_buffer, p_size, max_size));

    char line[1024];
    char output[DEFAULT_BUFFER_CAPACITY];
    for (int i = 0; i < n_entries; i++)
    {
        struct dir_entry_t* p_entry = &entries[i];
        int result = get_inode(p_entry->inode_id, &inode);
        
        
    }

    free(entries);
}

int make_file(const struct context_t *p_context, char filename[MAX_NAME_LEN])
{
}

int remove_file(const struct context_t *p_context, char filename[MAX_NAME_LEN])
{
}

int change_directory()
{
}

int make_directory()
{
}

int remove_directory()
{
}
