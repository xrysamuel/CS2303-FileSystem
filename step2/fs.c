#include "fs.h"
#include "error_type.h"
#include "fsconfig.h"
#include "inodes.h"
#include "buffer.h"
#include "blocks.h"

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

    // create root directory
    int inode_id;
    result = create_inode(&inode_id, MODE_UR | MODE_UW | MODE_UX | MODE_GR | MODE_GW | MODE_GX | MODE_OR | MODE_OW | MODE_OX | MODE_DIR, ROOT_UID, ROOT_GID);
    RET_ERR_RESULT(result);
    RET_ERR_IF(inode_id != ROOT_INODE_ID, , DEFAULT_ERROR);

    // create "..", ".", /home/, /passwd
    int home_inode_id, passwd_inode_id;
    result = create_inode(&passwd_inode_id, MODE_UR | MODE_UW | MODE_UX | MODE_GR | MODE_GX | MODE_OR | MODE_OX, ROOT_UID, ROOT_GID);
    RET_ERR_RESULT(result);
    RET_ERR_IF(passwd_inode_id != PASSWD_INODE_ID, , DEFAULT_ERROR);
    result = create_inode(&home_inode_id, MODE_UR | MODE_UW | MODE_UX | MODE_GR | MODE_GW | MODE_GX | MODE_OR | MODE_OW | MODE_OX | MODE_DIR, ROOT_UID, ROOT_GID);
    RET_ERR_RESULT(result);

    result = inode_file_resize(inode_id, 4 * DIR_ENTRY_SIZE);
    RET_ERR_RESULT(result);
    struct dir_entry_t root_children[4] = {{"..\0", inode_id}, {".\0", inode_id}, {"home\0", home_inode_id}, {"passwd\0", passwd_inode_id}};
    result = inode_file_write(inode_id, (char *)root_children, 0, 4 * DIR_ENTRY_SIZE);
    RET_ERR_RESULT(result);

    // create "..", "." for /home/
    result = inode_file_resize(home_inode_id, 2 * DIR_ENTRY_SIZE);
    RET_ERR_RESULT(result);
    struct dir_entry_t parent_and_self[2] = {{"..\0", inode_id}, {".\0", home_inode_id}};
    result = inode_file_write(home_inode_id, (char *)parent_and_self, 0, 2 * DIR_ENTRY_SIZE);
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
        return str_to_buffer("Error: Permission denied.", arg.res_buffer, arg.p_res_size, arg.max_size);
    else if (result == NOT_FOUND)
        return str_to_buffer("Error: Not found.", arg.res_buffer, arg.p_res_size, arg.max_size);
    else if (result == ALREADY_EXISTS)
        return str_to_buffer("Error: Already exists.", arg.res_buffer, arg.p_res_size, arg.max_size);
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

    // authorize
    result = authorize(arg.p_context, &inode, auth);
    RET_ERR_IF(IS_ERROR(result), free(entries), error_response(result, arg));

    // do some operations
    *arg.p_res_size = 0;
    result = fs_op(arg, &n_entries, &entries);
    RET_ERR_IF(IS_ERROR(result), free(entries), error_response(result, arg));

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
    for (int i = 0; i < *p_n_entries; i++)
    {
        struct inode_t inode;
        result = get_inode((*p_entries)[i].inode_id, &inode);
        RET_ERR_RESULT(result);
        if (strncmp((*p_entries)[i].name, arg.req_buffer, arg.req_size) == 0 && !IS_MODE(MODE_DIR, inode.mode))
        {
            return ALREADY_EXISTS;
        }
    }

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

    result = str_to_buffer("Success.", arg.res_buffer, arg.p_res_size, arg.max_size);
    RET_ERR_RESULT(result);
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
    int result;
    for (int i = 0; i < *p_n_entries; i++)
    {
        struct inode_t inode;
        result = get_inode((*p_entries)[i].inode_id, &inode);
        RET_ERR_RESULT(result);
        if (strncmp((*p_entries)[i].name, arg.req_buffer, arg.req_size) == 0 && IS_MODE(MODE_DIR, inode.mode))
        {
            return ALREADY_EXISTS;
        }
    }

    // create dir inode
    int dir_inode_id;
    result = create_inode(&dir_inode_id, MODE_UR | MODE_UW | MODE_UX | MODE_GR | MODE_GX | MODE_OR | MODE_OX | MODE_DIR, arg.p_context->uid, arg.p_context->gid);
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

    result = str_to_buffer("Success.", arg.res_buffer, arg.p_res_size, arg.max_size);
    RET_ERR_RESULT(result);
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

            result = str_to_buffer("Success.", arg.res_buffer, arg.p_res_size, arg.max_size);
            RET_ERR_RESULT(result);
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
                if (MATCHES_QUERY((*p_entries)[i].name, field_buffer, field_size) && IS_MODE(MODE_DIR, inode.mode))
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
            for (int i = 0; i < *p_n_entries; i++)
            {
                struct inode_t inode;
                result = get_inode((*p_entries)[i].inode_id, &inode);
                RET_ERR_RESULT(result);
                if (MATCHES_QUERY((*p_entries)[i].name, field_buffer, field_size) && IS_MODE(MODE_DIR, inode.mode))
                {
                    // authorize
                    result = authorize(arg.p_context, &inode, READ_AUTH);
                    RET_ERR_IF(IS_ERROR(result), , result);

                    int saved_cur_inode_id = arg.p_context->cur_inode_id;
                    arg.p_context->cur_inode_id = (*p_entries)[i].inode_id;
                    struct response_arg_t sub_arg = {arg.p_context, arg.res_buffer, arg.p_res_size, arg.max_size, arg.req_buffer + field_size + 1, arg.req_size - field_size - 1}; // "path/to/target" -> "to/target"
                    result = fs_operation_wrapper(change_dir, sub_arg, READ_AUTH);
                    // roll back
                    RET_ERR_IF(IS_ERROR(result), arg.p_context->cur_inode_id = saved_cur_inode_id, result);
                    return SUCCESS;
                }
            }
            return NOT_FOUND;
        }
    }
    return SUCCESS;
}

int catch_file_kernel(int inode_id, int size, char *data_buffer, int *data_size, int max_data_size)
{
    RET_ERR_IF(size > max_data_size, , BUFFER_OVERFLOW);
    int result = inode_file_read(inode_id, data_buffer, 0, size);
    RET_ERR_RESULT(result);
    *data_size = size;
    return SUCCESS;
}

int catch_file(struct response_arg_t arg, int *p_n_entries, struct dir_entry_t **p_entries)
{
    int result;
    RET_ERR_IF(arg.req_size >= MAX_NAME_LEN, , BUFFER_OVERFLOW);

    for (int i = 0; i < *p_n_entries; i++)
    {
        struct inode_t inode;
        int inode_id = (*p_entries)[i].inode_id;
        result = get_inode(inode_id, &inode);
        RET_ERR_RESULT(result);
        if (MATCHES_QUERY((*p_entries)[i].name, arg.req_buffer, arg.req_size) && !IS_MODE(MODE_DIR, inode.mode))
        {
            // authorize
            result = authorize(arg.p_context, &inode, READ_AUTH);
            RET_ERR_RESULT(result);

            // read file
            result = catch_file_kernel(inode_id, inode.size, arg.res_buffer, arg.p_res_size, arg.max_size);
            RET_ERR_RESULT(result);
            return SUCCESS;
        }
    }
    return NOT_FOUND;
}

int write_file_kernel(int inode_id, int len, const char *data_buffer)
{
    int result;
    // resize
    result = inode_file_resize(inode_id, len);
    RET_ERR_RESULT(result);

    // write file
    result = inode_file_write(inode_id, data_buffer, 0, len);
    RET_ERR_RESULT(result);

    return SUCCESS;
}

int write_file(struct response_arg_t arg, int *p_n_entries, struct dir_entry_t **p_entries)
{
    int result;

    // parse: <filename> <#len> <data>
    char *req_buffer = (char *)malloc(arg.req_size);
    RET_ERR_IF(req_buffer == NULL, , BAD_ALLOC_ERROR);
    memcpy(req_buffer, arg.req_buffer, arg.req_size);
    int req_size = arg.req_size;

    char *data_buffer, *len_buffer;
    int data_size, len_size;
    result = cut_at_n_space(req_buffer, req_size, 2, &data_buffer, &req_size, &data_size);
    RET_ERR_IF(IS_ERROR(result), free(req_buffer), result);
    result = cut_at_n_space(req_buffer, req_size, 1, &len_buffer, &req_size, &len_size);
    RET_ERR_IF(IS_ERROR(result), free(req_buffer), result);

    int len = atoi(len_buffer);
    RET_ERR_IF(len != data_size, free(req_buffer), DEFAULT_ERROR);
    RET_ERR_IF(req_size >= MAX_NAME_LEN, free(req_buffer), BUFFER_OVERFLOW);

    for (int i = 0; i < *p_n_entries; i++)
    {
        struct inode_t inode;
        int inode_id = (*p_entries)[i].inode_id;
        result = get_inode(inode_id, &inode);
        RET_ERR_IF(IS_ERROR(result), free(req_buffer), result);

        if (MATCHES_QUERY((*p_entries)[i].name, req_buffer, req_size) && !IS_MODE(MODE_DIR, inode.mode))
        {
            // authorize
            result = authorize(arg.p_context, &inode, WRITE_AUTH);
            RET_ERR_IF(IS_ERROR(result), free(req_buffer), result);

            // write
            result = write_file_kernel(inode_id, len, data_buffer);
            RET_ERR_IF(IS_ERROR(result), free(req_buffer), result);

            free(req_buffer);

            result = str_to_buffer("Success.", arg.res_buffer, arg.p_res_size, arg.max_size);
            RET_ERR_RESULT(result);
            return SUCCESS;
        }
    }
    free(req_buffer);
    return NOT_FOUND;
}

int chmod_file(struct response_arg_t arg, int *p_n_entries, struct dir_entry_t **p_entries)
{
    int result;

    // parse: <filename> <#mode>
    char *req_buffer = (char *)malloc(arg.req_size);
    RET_ERR_IF(req_buffer == NULL, , BAD_ALLOC_ERROR);
    memcpy(req_buffer, arg.req_buffer, arg.req_size);
    int req_size = arg.req_size;

    char *mode_buffer;
    int mode_size;
    result = cut_at_n_space(req_buffer, req_size, 1, &mode_buffer, &req_size, &mode_size);
    RET_ERR_IF(IS_ERROR(result), free(req_buffer), result);

    int mode = atoi(mode_buffer);
    RET_ERR_IF(req_size >= MAX_NAME_LEN, free(req_buffer), BUFFER_OVERFLOW);

    for (int i = 0; i < *p_n_entries; i++)
    {
        struct inode_t inode;
        int inode_id = (*p_entries)[i].inode_id;
        result = get_inode(inode_id, &inode);
        RET_ERR_IF(IS_ERROR(result), free(req_buffer), result);

        if (MATCHES_QUERY((*p_entries)[i].name, req_buffer, req_size) && !IS_MODE(MODE_DIR, inode.mode))
        {
            // authorize
            result = authorize(arg.p_context, &inode, WRITE_AUTH);
            RET_ERR_IF(IS_ERROR(result), free(req_buffer), result);

            // change mode
            inode.mode = mode;

            // write inode
            result = write_inode(inode_id, &inode);
            RET_ERR_IF(IS_ERROR(result), free(req_buffer), result);

            result = str_to_buffer("Success.", arg.res_buffer, arg.p_res_size, arg.max_size);
            RET_ERR_RESULT(result);
            return SUCCESS;
        }
    }
    free(req_buffer);
    return NOT_FOUND;
}

int insert_file_kernel(int inode_id, int size, int pos, int len, const char *data_buffer)
{
    int result;
    // resize
    pos = (pos > size) ? size : pos;
    int target_size = size + len;
    result = inode_file_resize(inode_id, target_size);
    RET_ERR_IF(IS_ERROR(result), , result);

    // read file
    char *buffer = (char *)malloc(size - pos);
    RET_ERR_IF(buffer == NULL, , BAD_ALLOC_ERROR);
    result = inode_file_read(inode_id, buffer, pos, size - pos);
    RET_ERR_IF(IS_ERROR(result), free(buffer), result);

    // write file
    result = inode_file_write(inode_id, data_buffer, pos, len);
    RET_ERR_IF(IS_ERROR(result), free(buffer), result);
    result = inode_file_write(inode_id, buffer, pos + len, size - pos);
    RET_ERR_IF(IS_ERROR(result), free(buffer), result);

    free(buffer);
    return SUCCESS;
}

int insert_file(struct response_arg_t arg, int *p_n_entries, struct dir_entry_t **p_entries)
{
    int result;

    // parse: <filename> <#pos> <#len> <data>
    char *req_buffer = (char *)malloc(arg.req_size);
    RET_ERR_IF(req_buffer == NULL, , BAD_ALLOC_ERROR);
    memcpy(req_buffer, arg.req_buffer, arg.req_size);
    int req_size = arg.req_size;

    char *data_buffer, *len_buffer, *pos_buffer;
    int data_size, len_size, pos_size;

    result = cut_at_n_space(req_buffer, req_size, 3, &data_buffer, &req_size, &data_size);
    RET_ERR_IF(IS_ERROR(result), free(req_buffer), result);
    result = cut_at_n_space(req_buffer, req_size, 2, &len_buffer, &req_size, &len_size);
    RET_ERR_IF(IS_ERROR(result), free(req_buffer), result);
    result = cut_at_n_space(req_buffer, req_size, 1, &pos_buffer, &req_size, &pos_size);
    RET_ERR_IF(IS_ERROR(result), free(req_buffer), result);

    int len = atoi(len_buffer);
    int pos = atoi(pos_buffer);
    RET_ERR_IF(len != data_size || pos < 0, free(req_buffer), DEFAULT_ERROR);
    RET_ERR_IF(req_size >= MAX_NAME_LEN, free(req_buffer), BUFFER_OVERFLOW);

    for (int i = 0; i < *p_n_entries; i++)
    {
        struct inode_t inode;
        int inode_id = (*p_entries)[i].inode_id;
        result = get_inode(inode_id, &inode);
        RET_ERR_IF(IS_ERROR(result), free(req_buffer), result);

        if (MATCHES_QUERY((*p_entries)[i].name, req_buffer, req_size) && !IS_MODE(MODE_DIR, inode.mode))
        {
            // authorize
            result = authorize(arg.p_context, &inode, WRITE_AUTH);
            RET_ERR_IF(IS_ERROR(result), free(req_buffer), result);

            // insert file
            result = insert_file_kernel(inode_id, inode.size, pos, len, data_buffer);
            RET_ERR_IF(IS_ERROR(result), free(req_buffer), result);

            free(req_buffer);

            result = str_to_buffer("Success.", arg.res_buffer, arg.p_res_size, arg.max_size);
            RET_ERR_RESULT(result);
            return SUCCESS;
        }
    }
    free(req_buffer);
    return NOT_FOUND;
}

int delete_in_file_kernel(int inode_id, int size, int pos, int len)
{
    int result;

    int end = pos + len;
    end = (end > size) ? size : end;
    len = end - pos;
    int target_size = size - len;

    // read file
    char *buffer = (char *)malloc(size - end);
    RET_ERR_IF(buffer == NULL, , BAD_ALLOC_ERROR);
    result = inode_file_read(inode_id, buffer, end, size - end);
    RET_ERR_IF(IS_ERROR(result), free(buffer), result);

    // write file
    result = inode_file_write(inode_id, buffer, pos, size - end);
    RET_ERR_IF(IS_ERROR(result), free(buffer), result);

    // resize
    result = inode_file_resize(inode_id, target_size);
    RET_ERR_IF(IS_ERROR(result), free(buffer), result);

    free(buffer);
    return SUCCESS;
}

int delete_in_file(struct response_arg_t arg, int *p_n_entries, struct dir_entry_t **p_entries)
{
    int result;

    // parse: <filename> <#pos> <#len>
    char *req_buffer = (char *)malloc(arg.req_size);
    RET_ERR_IF(req_buffer == NULL, , BAD_ALLOC_ERROR);
    memcpy(req_buffer, arg.req_buffer, arg.req_size);
    int req_size = arg.req_size;

    char *len_buffer, *pos_buffer;
    int len_size, pos_size;
    result = cut_at_n_space(req_buffer, req_size, 2, &len_buffer, &req_size, &len_size);
    RET_ERR_IF(IS_ERROR(result), free(req_buffer), result);
    result = cut_at_n_space(req_buffer, req_size, 1, &pos_buffer, &req_size, &pos_size);
    RET_ERR_IF(IS_ERROR(result), free(req_buffer), result);

    int len = atoi(len_buffer);
    int pos = atoi(pos_buffer);
    RET_ERR_IF(len < 0 || pos < 0, free(req_buffer), DEFAULT_ERROR);
    RET_ERR_IF(req_size >= MAX_NAME_LEN, free(req_buffer), BUFFER_OVERFLOW);

    for (int i = 0; i < *p_n_entries; i++)
    {
        struct inode_t inode;
        int inode_id = (*p_entries)[i].inode_id;
        result = get_inode(inode_id, &inode);
        RET_ERR_IF(IS_ERROR(result), free(req_buffer), result);

        if (MATCHES_QUERY((*p_entries)[i].name, req_buffer, req_size) && !IS_MODE(MODE_DIR, inode.mode))
        {
            // authorize
            result = authorize(arg.p_context, &inode, WRITE_AUTH);
            RET_ERR_IF(IS_ERROR(result), free(req_buffer), result);

            // delete in file
            result = delete_in_file_kernel(inode_id, inode.size, pos, len);
            RET_ERR_IF(IS_ERROR(result), free(req_buffer), result);

            free(req_buffer);

            result = str_to_buffer("Success.", arg.res_buffer, arg.p_res_size, arg.max_size);
            RET_ERR_RESULT(result);
            return SUCCESS;
        }
    }
    free(req_buffer);
    return NOT_FOUND;
}

int change_account(struct response_arg_t arg, int *p_n_entries, struct dir_entry_t **p_entries)
{
    int result;

    // parse <account name> <password>
    char *req_buffer = (char *)malloc(arg.req_size);
    RET_ERR_IF(req_buffer == NULL, , BAD_ALLOC_ERROR);
    memcpy(req_buffer, arg.req_buffer, arg.req_size);
    int req_size = arg.req_size;

    char *password_buffer;
    int password_size;
    result = cut_at_n_space(req_buffer, req_size, 1, &password_buffer, &req_size, &password_size);
    RET_ERR_IF(IS_ERROR(result), free(req_buffer), result);

    RET_ERR_IF(req_size >= MAX_USERNAME_LEN, free(req_buffer), result);
    RET_ERR_IF(password_size >= MAX_PASSWORD_LEN, free(req_buffer), result);

    // get the inode of passwd
    struct inode_t passwd_inode;
    result = get_inode(PASSWD_INODE_ID, &passwd_inode);
    RET_ERR_IF(IS_ERROR(result), free(req_buffer), result);

    int n_acc_entries = passwd_inode.size / ACC_ENTRY_SIZE;
    struct acc_entry_t *acc_entries = (struct acc_entry_t *)malloc(passwd_inode.size);
    RET_ERR_IF(acc_entries == NULL, free(req_buffer), BAD_ALLOC_ERROR);

    result = inode_file_read(PASSWD_INODE_ID, (char *)acc_entries, 0, passwd_inode.size);
    RET_ERR_IF(IS_ERROR(result), free(req_buffer); free(acc_entries), result);

    // search for account
    for (int i = 0; i < n_acc_entries; i++)
    {
        if (MATCHES_QUERY(acc_entries[i].name, req_buffer, req_size) && acc_entries[i].uid == i)
        {
            if (MATCHES_QUERY(acc_entries[i].password, password_buffer, password_size))
            {
                arg.p_context->uid = i;
                arg.p_context->gid = acc_entries[i].gid;
                free(req_buffer);
                free(acc_entries);

                result = str_to_buffer("Change to existed account.", arg.res_buffer, arg.p_res_size, arg.max_size);
                RET_ERR_RESULT(result);
                return SUCCESS;
            }
            else
            {
                free(req_buffer);
                free(acc_entries);
                return PERMISSION_DENIED;
            }
        }
    }

    // not found, then append
    struct acc_entry_t new_acc;
    new_acc.gid = 0;
    new_acc.uid = n_acc_entries;
    result = buffer_to_str(req_buffer, req_size, new_acc.name, MAX_NAME_LEN);
    RET_ERR_IF(IS_ERROR(result), free(req_buffer); free(acc_entries), result);
    result = buffer_to_str(password_buffer, password_size, new_acc.password, MAX_NAME_LEN);
    RET_ERR_IF(IS_ERROR(result), free(req_buffer); free(acc_entries), result);
    result = insert_file_kernel(PASSWD_INODE_ID, passwd_inode.size, passwd_inode.size, ACC_ENTRY_SIZE, (char *)&new_acc);
    RET_ERR_IF(IS_ERROR(result), free(req_buffer); free(acc_entries), result);

    free(req_buffer);
    free(acc_entries);
    result = str_to_buffer("New account created.", arg.res_buffer, arg.p_res_size, arg.max_size);
    RET_ERR_RESULT(result);
    return SUCCESS;
}

int remove_account(struct response_arg_t arg, int *p_n_entries, struct dir_entry_t **p_entries)
{
    int result;

    // parse <account name> <password>
    char *req_buffer = (char *)malloc(arg.req_size);
    RET_ERR_IF(req_buffer == NULL, , BAD_ALLOC_ERROR);
    memcpy(req_buffer, arg.req_buffer, arg.req_size);
    int req_size = arg.req_size;

    char *password_buffer;
    int password_size;
    result = cut_at_n_space(req_buffer, req_size, 1, &password_buffer, &req_size, &password_size);
    RET_ERR_IF(IS_ERROR(result), free(req_buffer), result);

    RET_ERR_IF(req_size >= MAX_USERNAME_LEN, free(req_buffer), result);
    RET_ERR_IF(password_size >= MAX_PASSWORD_LEN, free(req_buffer), result);

    // get the inode of passwd
    struct inode_t passwd_inode;
    result = get_inode(PASSWD_INODE_ID, &passwd_inode);
    RET_ERR_IF(IS_ERROR(result), free(req_buffer), result);

    int n_acc_entries = passwd_inode.size / ACC_ENTRY_SIZE;
    struct acc_entry_t *acc_entries = (struct acc_entry_t *)malloc(passwd_inode.size);
    RET_ERR_IF(acc_entries == NULL, free(req_buffer), BAD_ALLOC_ERROR);

    result = inode_file_read(PASSWD_INODE_ID, (char *)acc_entries, 0, passwd_inode.size);
    RET_ERR_IF(IS_ERROR(result), free(req_buffer); free(acc_entries), result);

    // search for account
    for (int i = 0; i < n_acc_entries; i++)
    {
        if (MATCHES_QUERY(acc_entries[i].name, req_buffer, req_size) && acc_entries[i].uid == i)
        {
            if (MATCHES_QUERY(acc_entries[i].password, password_buffer, password_size))
            {
                // invalidate account
                acc_entries[i].uid = -1;
                result = inode_file_write(PASSWD_INODE_ID, (char *)acc_entries, 0, passwd_inode.size);
                RET_ERR_IF(IS_ERROR(result), free(req_buffer); free(acc_entries), result);

                free(req_buffer);
                free(acc_entries);

                result = str_to_buffer("Account removed.", arg.res_buffer, arg.p_res_size, arg.max_size);
                RET_ERR_RESULT(result);
                return SUCCESS;
            }
            else
            {
                free(req_buffer);
                free(acc_entries);
                return PERMISSION_DENIED;
            }
        }
    }
    free(req_buffer);
    free(acc_entries);
    return NOT_FOUND;
}