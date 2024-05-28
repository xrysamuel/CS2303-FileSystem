#include "buffer.h"
#include "common.h"
#include "error_type.h"

bool starts_with(const char *buffer, int size, const char *prefix)
{
    if (buffer == NULL || prefix == NULL || size < 0)
        return false;
    int prefix_length = strlen(prefix);
    if (prefix_length > size)
        return false;
    return strncmp(buffer, prefix, prefix_length) == 0;
}

int concat_buffer(char *target_buffer, int *p_target_size, int max_target_size, const char *src_buffer_1, int src_size_1, const char *src_buffer_2, int src_size_2)
{
    int target_size = src_size_1 + src_size_2;
    RET_ERR_IF(max_target_size < target_size, , BUFFER_OVERFLOW);

    memcpy(target_buffer, src_buffer_1, src_size_1);
    memcpy(target_buffer + src_size_1, src_buffer_2, src_size_2);
    *p_target_size = target_size;
    return target_size;
}

int buffer_to_str(const char *buffer, int size, char *str, int max_str_size)
{   
    RET_ERR_IF(buffer == NULL || str == NULL || size < 0 || max_str_size < 0, , INVALID_ARG_ERROR);
    RET_ERR_IF(size + 1 > max_str_size, , BUFFER_OVERFLOW);

    strncpy(str, buffer, size);
    for (int i = 0; i < size; i++)
    {
        if (str[i] == '\0')
        {
            str[i] = ' ';
        }
    }
    str[size] = '\0';
    return size + 1;
}

int buffer_to_hex_str(const char *buffer, int size, char *str, int max_str_size)
{
    RET_ERR_IF(buffer == NULL || str == NULL || size < 0 || max_str_size < 0, , INVALID_ARG_ERROR);
    int str_len = 8 * size;
    RET_ERR_IF(str_len > max_str_size, , BUFFER_OVERFLOW);

    for (int i = 0; i < size; i++)
    {
        if (isprint(buffer[i]))
            sprintf(str + (i * 8), "%02X('%c') ", (u_int8_t) buffer[i], buffer[i]);
        else
            sprintf(str + (i * 8), "%02X(' ') ", (u_int8_t) buffer[i]);
    }
    str[str_len] = '\0';
    return str_len;
}

int str_to_buffer(const char *str, char *buffer, int *p_size, int max_size)
{
    RET_ERR_IF(str == NULL || buffer == NULL || p_size == NULL || max_size < 0, , INVALID_ARG_ERROR);
    int size = strlen(str);
    RET_ERR_IF(size > max_size, , BUFFER_OVERFLOW);

    strncpy(buffer, str, size);
    *p_size = size;
    return size;
}