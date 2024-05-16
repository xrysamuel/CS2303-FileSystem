#include "buffer.h"
#include "std.h"
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

int concat_buffer(char **p_target_buffer, int *p_target_size, const char *src_buffer_1, int src_size_1, const char *src_buffer_2, int src_size_2)
{
    int target_size = src_size_1 + src_size_2;

    char *target_buffer = (char *)malloc(target_size);
    RET_ERR_IF(target_buffer == NULL, , BAD_ALLOC_ERROR);

    memcpy(target_buffer, src_buffer_1, src_size_1);
    memcpy(target_buffer + src_size_1, src_buffer_2, src_size_2);

    *p_target_buffer = target_buffer;
    *p_target_size = target_size;
    return target_size;
}

int buffer_to_str(const char *buffer, int size, char **p_str)
{
    RET_ERR_IF(buffer == NULL || p_str == NULL || size < 0, , INVALID_ARG_ERROR);

    char *str = (char *)malloc((size + 1) * sizeof(char));
    RET_ERR_IF(str == NULL, , BAD_ALLOC_ERROR);

    strncpy(str, buffer, size);
    for (int i = 0; i < size; i++)
    {
        if (str[i] == '\0')
        {
            str[i] = ' ';
        }
    }
    str[size] = '\0';

    *p_str = str;
    return size + 1;
}

int buffer_to_hex_str(const char *buffer, int size, char **p_str)
{
    RET_ERR_IF(buffer == NULL || p_str == NULL, , INVALID_ARG_ERROR);

    int str_len = 8 * size;
    char *str = (char *)malloc((str_len + 1) * sizeof(char));
    RET_ERR_IF(str == NULL, , BAD_ALLOC_ERROR);

    for (int i = 0; i < size; i++)
    {
        if (isprint(buffer[i]))
            sprintf(str + (i * 8), "%02X('%c') ", (u_int8_t) buffer[i], buffer[i]);
        else
            sprintf(str + (i * 8), "%02X(' ') ", (u_int8_t) buffer[i]);
    }

    str[str_len] = '\0';

    *p_str = str;
    return str_len;
}

int str_to_buffer(const char *str, char **p_buffer, int *p_size)
{
    RET_ERR_IF(str == NULL || p_buffer == NULL || p_size == NULL, , INVALID_ARG_ERROR);

    int size = strlen(str);
    char *buffer = (char *)malloc(size * sizeof(char));
    RET_ERR_IF(buffer == NULL, , BAD_ALLOC_ERROR);

    strncpy(buffer, str, size);

    *p_buffer = buffer;
    *p_size = size;
    return size;
}