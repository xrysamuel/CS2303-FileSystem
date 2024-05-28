#include "disk.h"
#include "std.h"
#include "error_type.h"
#include "client.h"
#include "buffer.h"
#include "definition.h"

response_t disk_server_response;
int n_cylinder;
int n_sectors;

typedef struct
{
    char data[BLOCK_SIZE];
} cache_entry_t;

cache_entry_t *cache;
int ref[CACHE_SIZE];
int blocks[CACHE_SIZE];
int victim;

int disk_read_direct(char buffer[BLOCK_SIZE], int block);

int disk_write_direct(const char buffer[BLOCK_SIZE], int block);

void disk_init(const char *server_ip, int port)
{
    custom_client_init(server_ip, port, &disk_server_response);

    // "I" -> res_buffer
    char *res_buffer = NULL;
    int res_size;
    int result = disk_server_response("I", 1, &res_buffer, &res_size);
    EXIT_IF(IS_ERROR(result), custom_client_close(), "Error: Could not get disk info.\n");

    // res_buffer -> res_str
    char *res_str = NULL;
    result = buffer_to_str(res_buffer, res_size, &res_str);
    free(res_buffer);
    EXIT_IF(IS_ERROR(result), custom_client_close(), "Error: Could not get disk info.\n");

    // res_str -> n_cylinders, n_sectors
    result = sscanf(res_str, "%d %d", &n_cylinder, &n_sectors);
    free(res_str);
    EXIT_IF(result != 2, custom_client_close(), "Error: Could not get disk info.\n");

    // cache init
    cache = (cache_entry_t *)malloc(CACHE_SIZE * sizeof(cache_entry_t));
    EXIT_IF(cache == NULL, custom_client_close(), "Error: Bad alloc.\n");

    for (int i = 0; i < CACHE_SIZE; i++)
    {
        ref[i] = -1;
        blocks[i] = -1;
        victim = 0;
    }
}

void disk_close()
{
    custom_client_close();

    // cache cleanup
    int result;
    for (int i = 0; i < CACHE_SIZE; i++)
    {
        if (ref[i] != -1)
        {
            result = -1;
            while (IS_ERROR(result))
            {
                result = disk_write_direct(cache[i].data, blocks[i]);
            }
        }
    }
    if (cache != NULL)
        free(cache);
}

void get_n_blocks(int *p_n_blocks)
{
    *p_n_blocks = n_cylinder * n_sectors;
}

int disk_read_direct(char buffer[BLOCK_SIZE], int block)
{
    // block -> cylinder, sector
    int cylinder = block / n_sectors;
    int sector = block % n_sectors;

    // cylinder, sector -> req_str
    char req_str[20];
    sprintf(req_str, "R %d %d", cylinder, sector);

    // req_str -> res_buffer
    char *res_buffer = NULL;
    int res_size;
    int result = disk_server_response(req_str, strlen(req_str), &res_buffer, &res_size);
    RET_ERR_RESULT(result); 
    RET_ERR_IF(res_size != 4 + BLOCK_SIZE, , READ_ERROR);
    RET_ERR_IF(!starts_with(res_buffer, res_size, "Yes"), , READ_ERROR);

    // res_buffer -> buffer
    memcpy(buffer, res_buffer + 4, BLOCK_SIZE);
    free(res_buffer);
    return BLOCK_SIZE;
}

int disk_write_direct(const char buffer[BLOCK_SIZE], int block)
{
    // block -> cylinder, sector
    int cylinder = block / n_sectors;
    int sector = block % n_sectors;

    // cylinder, sector -> req_head_str
    char req_head_str[30];
    sprintf(req_head_str, "W %d %d %d ", cylinder, sector, BLOCK_SIZE);

    // req_head_str, buffer -> req_buffer
    char *req_buffer = NULL;
    int req_size;
    int result = concat_buffer(&req_buffer, &req_size, req_head_str, strlen(req_head_str), buffer, BLOCK_SIZE);
    RET_ERR_RESULT(result); 

    // req_buffer -> res_buffer
    char *res_buffer = NULL;
    int res_size;
    result = disk_server_response(req_buffer, req_size, &res_buffer, &res_size);
    free(req_buffer);
    RET_ERR_RESULT(result); 
    RET_ERR_IF(!starts_with(res_buffer, res_size, "Yes"), , READ_ERROR);
    return res_size;
}

int disk_read(char buffer[BLOCK_SIZE], int block)
{
    for (int i = 0; i < CACHE_SIZE; i++)
    {
        if (blocks[i] == block)
        {
            // cache hit
            memcpy(buffer, cache[i].data, BLOCK_SIZE);
            ref[i] = 1;
            return BLOCK_SIZE;
        }
    }
    // cache miss
    int cur_ref, result;
    while ((cur_ref = ref[victim]) == 1)
    {
        cur_ref = 0;
        victim = (victim + 1) % CACHE_SIZE;
    }
    if (cur_ref == 0)
    {
        result = disk_write_direct(cache[victim].data, blocks[victim]);
        RET_ERR_RESULT(result); 
    }
    result = disk_read_direct(cache[victim].data, block);
    RET_ERR_RESULT(result); 
    blocks[victim] = block;
    ref[victim] = 1;
    memcpy(buffer, cache[victim].data, BLOCK_SIZE);
    victim = (victim + 1) % CACHE_SIZE;
    return BLOCK_SIZE;
}

int disk_write(const char buffer[BLOCK_SIZE], int block)
{
    for (int i = 0; i < CACHE_SIZE; i++)
    {
        if (blocks[i] == block)
        {
            // cache hit
            memcpy(cache[i].data, buffer, BLOCK_SIZE);
            ref[i] = 1;
            return BLOCK_SIZE;
        }
    }
    // cache miss
    int cur_ref, result;
    while ((cur_ref = ref[victim]) == 1)
    {
        cur_ref = 0;
        victim = (victim + 1) % CACHE_SIZE;
    }
    if (cur_ref == 0)
    {
        result = disk_write_direct(cache[victim].data, blocks[victim]);
        RET_ERR_RESULT(result); 
    }
    result = disk_read_direct(cache[victim].data, block);
    RET_ERR_RESULT(result); 
    blocks[victim] = block;
    ref[victim] = 1;
    memcpy(cache[victim].data, buffer, BLOCK_SIZE);
    victim = (victim + 1) % CACHE_SIZE;
    return BLOCK_SIZE;
}
