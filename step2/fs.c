#include "fs.h"
#include "error_type.h"
#include "fsconfig.h"
#include "inodes.h"
#include "buffer.h"

int cur_inode_id;

void fs_close()
{
    inodes_close();
}

void fs_init(const char *server_ip, int port)
{
    inodes_init(server_ip, port);
    cur_inode_id = ROOT_INODE_ID;
}

int fs_format()
{
    int result = inodes_format();
    RET_ERR_RESULT(result);

    int inode_id;

    // create root directory
    result = create_inode(&inode_id, MODE_UR | MODE_UW | MODE_UX | MODE_GR | MODE_GW | MODE_GX | MODE_OR | MODE_OW | MODE_OX | MODE_DIR, ROOT_UID, ROOT_GID);
    RET_ERR_RESULT(result);
    RET_ERR_IF(inode_id != ROOT_INODE_ID, , DEFAULT_ERROR);

    // create .. and . and /home
    int home_inode_id;
    result = create_inode(&home_inode_id, MODE_UR | MODE_UW | MODE_UX | MODE_GR | MODE_GX | MODE_OR | MODE_OX | MODE_DIR, ROOT_UID, ROOT_GID);
    RET_ERR_RESULT(result);
    result = inode_file_resize(inode_id, 2 * DIR_ENTRY_SIZE);
    RET_ERR_RESULT(result);
    struct dir_entry_t parent_and_self[2] = {{"..\0", inode_id}, {".\0", inode_id}};
    result = inode_file_write(inode_id, (char *)parent_and_self, 0, 2 * DIR_ENTRY_SIZE);
    RET_ERR_RESULT(result);

    return SUCCESS;
}

int error_response(int result, struct response_arg_t arg)
{
    if (result == READ_ERROR)
        return str_to_buffer("Error: Read error.", arg.res_buffer, arg.p_res_size, arg.max_size);
    else if (result == WRITE_ERROR)
        return str_to_buffer("Error: Write error.", arg.res_buffer, arg.p_res_size, arg.max_size);
    else if (result == DISK_FULL_ERROR)
        return str_to_buffer("Error: Disk is full.", arg.res_buffer, arg.p_res_size, arg.max_size);
    else if (result == PERMISSION_DENIED)
        return str_to_buffer("Error: Permission denies.", arg.res_buffer, arg.p_res_size, arg.max_size);
    else if (result == NOT_FOUND)
        return str_to_buffer("Error: Not found.", arg.res_buffer, arg.p_res_size, arg.max_size);
    else
        return str_to_buffer("Error.", arg.res_buffer, arg.p_res_size, arg.max_size);
}

int authorize(const struct context_t *p_context, struct inode_t *p_inode, enum auth_t auth)
{
    if (p_context == NULL || p_inode == NULL)
        return PERMISSION_DENIED;
    u_int16_t mode = p_inode->mode;
    u_int32_t uid = p_context->uid;
    u_int32_t gid = p_context->gid;
    switch (auth)
    {
    case READ_AUTH:
        if (uid == p_inode->uid && IS_MODE(mode, MODE_UR))
            return SUCCESS;
        if (gid == p_inode->gid && IS_MODE(mode, MODE_GR))
            return SUCCESS;
        if (IS_MODE(mode, MODE_OR))
            return SUCCESS;
        break;
    case WRITE_AUTH:
        if (uid == p_inode->uid && IS_MODE(mode, MODE_UW))
            return SUCCESS;
        if (gid == p_inode->gid && IS_MODE(mode, MODE_GW))
            return SUCCESS;
        if (IS_MODE(mode, MODE_OW))
            return SUCCESS;
        break;
    case EXE_AUTH:
        if (uid == p_inode->uid && IS_MODE(mode, MODE_UX))
            return SUCCESS;
        if (gid == p_inode->gid && IS_MODE(mode, MODE_GX))
            return SUCCESS;
        if (IS_MODE(mode, MODE_OX))
            return SUCCESS;
        break;
    case NO_AUTH:
        return SUCCESS;
        break;
    }
    return PERMISSION_DENIED;
}

int fs_operation_wrapper(fs_op_t fs_op, struct response_arg_t arg, enum auth_t auth)
{
    int result;
    struct dir_entry_t entries_for_debug[64];

    // get the inode of current working directory
    struct inode_t inode;
    int cur_inode_id = arg.p_context->cur_inode_id;
    result = get_inode(cur_inode_id, &inode);
    RET_ERR_IF(IS_ERROR(result), arg.p_context->cur_inode_id = ROOT_INODE_ID, error_response(result, arg));

    // get entries in current working directory
    int n_entries = inode.size / DIR_ENTRY_SIZE;
    struct dir_entry_t *entries = (struct dir_entry_t *)malloc(inode.size);
    RET_ERR_IF(entries == NULL, , BAD_ALLOC_ERROR);

    result = inode_file_read(cur_inode_id, (char *)entries, 0, inode.size);
    RET_ERR_IF(IS_ERROR(result), free(entries), error_response(result, arg));

    for (int i = 0; i < n_entries; i++)
    {
        entries_for_debug[i] = entries[i];
    }

    // authorize
    result = authorize(arg.p_context, &inode, auth);
    RET_ERR_IF(IS_ERROR(result), free(entries), error_response(result, arg));

    // do some operations
    result = fs_op(arg, &n_entries, &entries);
    RET_ERR_IF(IS_ERROR(result), free(entries), error_response(result, arg));

    for (int i = 0; i < n_entries; i++)
    {
        entries_for_debug[i] = entries[i];
    }

    // n_entries, entries may has been changed
    // save the changes !!!
    result = inode_file_resize(cur_inode_id, n_entries * DIR_ENTRY_SIZE);
    RET_ERR_IF(IS_ERROR(result), free(entries), error_response(result, arg));

    result = inode_file_write(cur_inode_id, (char *)entries, 0, n_entries * DIR_ENTRY_SIZE);
    RET_ERR_IF(IS_ERROR(result), free(entries), error_response(result, arg));

    free(entries);
    return SUCCESS;
}

int list(struct response_arg_t arg, int *p_n_entries, struct dir_entry_t **p_entries)
{
    int total_size = 0;
    int result;
    for (int i = 0; i < *p_n_entries; i++)
    {
        struct inode_t inode;
        result = get_inode((*p_entries)[i].inode_id, &inode);
        RET_ERR_RESULT(result);

        char mode_str[11];
        sprintf(mode_str, "%c%c%c%c%c%c%c%c%c%c",
                IS_MODE(inode.mode, MODE_DIR) ? 'd' : '-',
                IS_MODE(inode.mode, MODE_UR) ? 'r' : '-',
                IS_MODE(inode.mode, MODE_UW) ? 'w' : '-',
                IS_MODE(inode.mode, MODE_UX) ? 'x' : '-',
                IS_MODE(inode.mode, MODE_GR) ? 'r' : '-',
                IS_MODE(inode.mode, MODE_GW) ? 'w' : '-',
                IS_MODE(inode.mode, MODE_GX) ? 'x' : '-',
                IS_MODE(inode.mode, MODE_OR) ? 'r' : '-',
                IS_MODE(inode.mode, MODE_OW) ? 'w' : '-',
                IS_MODE(inode.mode, MODE_OX) ? 'x' : '-');

        char time_str[30];
        time_t timestamp = inode.mtime;
        struct tm *timeinfo = localtime(&timestamp);
        strftime(time_str, 30, "%b %d %H:%M", timeinfo);

        int n_print = snprintf(arg.res_buffer + total_size, arg.max_size - total_size,
                               "%s %5hu %5hu %10u %s %s\n",
                               mode_str, inode.gid, inode.uid, inode.size, time_str, (*p_entries)[i].name);

        if (n_print >= arg.max_size - total_size)
        {
            *arg.p_res_size = strlen(arg.res_buffer);
            return BUFFER_OVERFLOW;
        }

        total_size += n_print;
    }
    *arg.p_res_size = strlen(arg.res_buffer);
    return SUCCESS;
}

int make_file(struct response_arg_t arg, int *p_n_entries, struct dir_entry_t **p_entries)
{
    int result;
    // create file
    int file_inode_id;
    result = create_inode(&file_inode_id, MODE_UR | MODE_UW | MODE_UX | MODE_GR | MODE_GX | MODE_OR | MODE_OX, arg.p_context->uid, arg.p_context->gid);
    RET_ERR_RESULT(result);

    // add to entries of cwd
    struct dir_entry_t *new_entries = (struct dir_entry_t *)malloc((*p_n_entries + 1) * DIR_ENTRY_SIZE);
    RET_ERR_IF(new_entries == NULL, , BAD_ALLOC_ERROR);
    memcpy(new_entries, *p_entries, *p_n_entries * DIR_ENTRY_SIZE);
    RET_ERR_IF(arg.req_size > MAX_NAME_LEN, , BUFFER_OVERFLOW);
    buffer_to_str(arg.req_buffer, arg.req_size, new_entries[*p_n_entries].name, MAX_NAME_LEN);
    new_entries[*p_n_entries].inode_id = file_inode_id;

    free(*p_entries);
    *p_entries = new_entries;
    *p_n_entries = *p_n_entries + 1;
    return SUCCESS;
}

int remove_file(struct response_arg_t arg, int *p_n_entries, struct dir_entry_t **p_entries)
{
    int result;

    for (int i = 0; i < *p_n_entries; i++)
    {
        struct inode_t inode;
        result = get_inode((*p_entries)[i].inode_id, &inode);
        RET_ERR_RESULT(result);
        if (strncmp((*p_entries)[i].name, arg.req_buffer, arg.req_size) == 0 && !IS_MODE(MODE_DIR, inode.mode))
        {
            // authorize
            result = authorize(arg.p_context, &inode, WRITE_AUTH);
            RET_ERR_RESULT(result);

            // delete file
            result = delete_inode((*p_entries)[i].inode_id);
            RET_ERR_RESULT(result);

            // remove its entry from cwd
            (*p_entries)[i] = (*p_entries)[*p_n_entries - 1];
            *p_n_entries = *p_n_entries - 1;
            return SUCCESS;
        }
    }
    return NOT_FOUND;
}

int make_dir(struct response_arg_t arg, int *p_n_entries, struct dir_entry_t **p_entries)
{
    // create dir inode
    int dir_inode_id;
    int result = create_inode(&dir_inode_id, MODE_UR | MODE_UW | MODE_UX | MODE_GR | MODE_GX | MODE_OR | MODE_OX | MODE_DIR, arg.p_context->uid, arg.p_context->gid);
    RET_ERR_RESULT(result);

    // create .. and .
    result = inode_file_resize(dir_inode_id, 2 * DIR_ENTRY_SIZE);
    RET_ERR_RESULT(result);
    struct dir_entry_t parent_and_self[2] = {{"..\0", arg.p_context->cur_inode_id}, {".\0", dir_inode_id}};
    result = inode_file_write(dir_inode_id, (char *)parent_and_self, 0, 2 * DIR_ENTRY_SIZE);
    RET_ERR_RESULT(result);

    // add to entries of cwd
    struct dir_entry_t *new_entries = (struct dir_entry_t *)malloc((*p_n_entries + 1) * DIR_ENTRY_SIZE);
    RET_ERR_IF(new_entries == NULL, , BAD_ALLOC_ERROR);
    memcpy(new_entries, *p_entries, *p_n_entries * DIR_ENTRY_SIZE);
    RET_ERR_IF(arg.req_size > MAX_NAME_LEN, , BUFFER_OVERFLOW);
    buffer_to_str(arg.req_buffer, arg.req_size, new_entries[*p_n_entries].name, MAX_NAME_LEN);
    new_entries[*p_n_entries].inode_id = dir_inode_id;

    free(*p_entries);
    *p_entries = new_entries;
    *p_n_entries = *p_n_entries + 1;
    return SUCCESS;
}

int remove_dir(struct response_arg_t arg, int *p_n_entries, struct dir_entry_t **p_entries)
{
    int result;

    for (int i = 0; i < *p_n_entries; i++)
    {
        struct inode_t inode;
        result = get_inode((*p_entries)[i].inode_id, &inode);
        RET_ERR_RESULT(result);
        if (strncmp((*p_entries)[i].name, arg.req_buffer, arg.req_size) == 0 && IS_MODE(MODE_DIR, inode.mode) && inode.size <= 2 * DIR_ENTRY_SIZE)
        {
            // authorize
            result = authorize(arg.p_context, &inode, WRITE_AUTH);
            RET_ERR_RESULT(result);

            // delete dir
            result = delete_inode((*p_entries)[i].inode_id);
            RET_ERR_RESULT(result);

            // remove its entry from cwd
            (*p_entries)[i] = (*p_entries)[*p_n_entries - 1];
            *p_n_entries = *p_n_entries - 1;
            return SUCCESS;
        }
    }
    return NOT_FOUND;
}

int change_dir(struct response_arg_t arg, int *p_n_entries, struct dir_entry_t **p_entries)
{
    // "/path/to/target" -> "path/to/target" -> "to/target" -> "target" -> SUCCESS
    // "path/to/target" -> "to/target" -> "target" -> SUCCESS
    // "/path/to/target/" -> "path/to/target/" -> "to/target/" -> "target/" -> "" -> SUCCESS
    // "path/to/target/" -> "to/target/" -> "target/" -> "" -> SUCCESS

    int result;

    if (arg.req_size == 0) // ""
    {
        return SUCCESS;
    }
    else if (starts_with(arg.req_buffer, arg.req_size, "/"))
    {
        int saved_cur_inode_id = arg.p_context->cur_inode_id;
        arg.p_context->cur_inode_id = 0;
        struct response_arg_t sub_arg = {arg.p_context, arg.res_buffer, arg.p_res_size, arg.max_size, arg.req_buffer + 1, arg.req_size - 1}; // "/path/to/target" -> "path/to/target"
        result = fs_operation_wrapper(change_dir, sub_arg, READ_AUTH);
        // roll back
        RET_ERR_IF(IS_ERROR(result), arg.p_context->cur_inode_id = saved_cur_inode_id, result);
        return SUCCESS;
    }
    else
    {
        char field_buffer[DEFAULT_BUFFER_CAPACITY];
        int field_size;
        buffer_get_first_field(arg.req_buffer, arg.req_size, field_buffer, &field_size, DEFAULT_BUFFER_CAPACITY, '/');
        if (field_size == arg.req_size) // "target"
        {
            for (int i = 0; i < *p_n_entries; i++)
            {
                struct inode_t inode;
                result = get_inode((*p_entries)[i].inode_id, &inode);
                RET_ERR_RESULT(result);
                if (strncmp((*p_entries)[i].name, field_buffer, MAX_NAME_LEN) == 0 && IS_MODE(MODE_DIR, inode.mode))
                {
                    result = authorize(arg.p_context, &inode, READ_AUTH);
                    RET_ERR_IF(IS_ERROR(result), , result);
                    arg.p_context->cur_inode_id = (*p_entries)[i].inode_id;
                    return SUCCESS;
                }
            }
            return NOT_FOUND;
        }
        else
        {
            int saved_cur_inode_id = arg.p_context->cur_inode_id;
            arg.p_context->cur_inode_id = 0;
            struct response_arg_t sub_arg = {arg.p_context, arg.res_buffer, arg.p_res_size, arg.max_size, arg.req_buffer + field_size + 1, arg.req_size - field_size - 1}; // "path/to/target" -> "to/target"
            result = fs_operation_wrapper(change_dir, sub_arg, READ_AUTH);
            // roll back
            RET_ERR_IF(IS_ERROR(result), arg.p_context->cur_inode_id = saved_cur_inode_id, result);
            return SUCCESS;
        }
    }
    return SUCCESS;
}

int catch_file(struct response_arg_t arg, int *p_n_entries, struct dir_entry_t **p_entries)
{
    int result;

    for (int i = 0; i < *p_n_entries; i++)
    {
        struct inode_t inode;
        result = get_inode((*p_entries)[i].inode_id, &inode);
        RET_ERR_RESULT(result);
        if (strncmp((*p_entries)[i].name, arg.req_buffer, arg.req_size) == 0 && !IS_MODE(MODE_DIR, inode.mode))
        {
            // authorize
            result = authorize(arg.p_context, &inode, READ_AUTH);
            RET_ERR_RESULT(result);

            // read file
            RET_ERR_IF(inode.size > arg.max_size, , BUFFER_OVERFLOW);
            result = inode_file_read((*p_entries)[i].inode_id, arg.res_buffer, 0, inode.size);
            RET_ERR_RESULT(result);
            *arg.p_res_size = inode.size;
            return SUCCESS;
        }
    }
    return NOT_FOUND;
}

int write_file(struct response_arg_t arg, int *p_n_entries, struct dir_entry_t **p_entries)
{
    int result;

    // parse
    char *req_buffer = (char *)malloc(arg.req_size);
    RET_ERR_IF(req_buffer == NULL, , BAD_ALLOC_ERROR);
    memcpy(req_buffer, arg.req_buffer, arg.req_size);
    int req_size = arg.req_size;

    char *data_buffer;
    int data_size;
    result = cut_at_n_space(req_buffer, arg.req_size, 2, &data_buffer, &req_size, &data_size);
    RET_ERR_IF(IS_ERROR(result), free(req_buffer), result);

    char *len_buffer;
    int len_size;
    result = cut_at_n_space(req_buffer, req_size, 1, &len_buffer, &req_size, &len_size);
    RET_ERR_IF(IS_ERROR(result), free(req_buffer), result);

    // check len
    int len = atoi(len_buffer);
    RET_ERR_IF(len != data_size, free(req_buffer), DEFAULT_ERROR);

    for (int i = 0; i < *p_n_entries; i++)
    {
        struct inode_t inode;
        result = get_inode((*p_entries)[i].inode_id, &inode);
        RET_ERR_IF(IS_ERROR(result), free(req_buffer), result);

        if (strncmp((*p_entries)[i].name, req_buffer, req_size) == 0 && !IS_MODE(MODE_DIR, inode.mode))
        {
            // authorize
            result = authorize(arg.p_context, &inode, WRITE_AUTH);
            RET_ERR_IF(IS_ERROR(result), free(req_buffer), result);

            // resize
            result = inode_file_resize((*p_entries)[i].inode_id, len);
            RET_ERR_IF(IS_ERROR(result), free(req_buffer), result);

            // write file
            result = inode_file_write((*p_entries)[i].inode_id, data_buffer, 0, len);
            RET_ERR_IF(IS_ERROR(result), free(req_buffer), result);
            int for_debug = (*p_entries)[i].inode_id;

            return SUCCESS;
        }
    }
    return NOT_FOUND;
}