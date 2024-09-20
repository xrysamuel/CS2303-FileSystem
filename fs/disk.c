#include "disk.h"
#include "common.h"
#include "error_type.h"
#include "client.h"
#include "buffer.h"
#include "fsconfig.h"

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
    printf("disk: initializing\n");
    custom_client_init(server_ip, port, &disk_server_response);

    // "I" -> res_buffer
    char res_buffer[DEFAULT_BUFFER_CAPACITY];
    int res_size;
    int result = disk_server_response(0, "I", 1, res_buffer, &res_size, DEFAULT_BUFFER_CAPACITY);
    EXIT_IF(IS_ERROR(result), custom_client_close(), "Error: Could not get disk info.\n");

    // res_buffer -> res_str
    char res_str[DEFAULT_BUFFER_CAPACITY];
    result = buffer_to_str(res_buffer, res_size, res_str, DEFAULT_BUFFER_CAPACITY);
    EXIT_IF(IS_ERROR(result), custom_client_close(), "Error: Could not get disk info.\n");

    // res_str -> n_cylinders, n_sectors
    result = sscanf(res_str, "%d %d", &n_cylinder, &n_sectors);
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
    printf("disk: closing\n");

    // cache cleanup
    for (int i = 0; i < CACHE_SIZE; i++)
    {
        if (ref[i] != -1)
        {
            disk_write_direct(cache[i].data, blocks[i]);
        }
    }
    if (cache != NULL)
        free(cache);
    custom_client_close();
}

void get_n_blocks(int *p_n_blocks)
{
    *p_n_blocks = n_cylinder * n_sectors;
}

int disk_read_direct(char buffer[BLOCK_SIZE], int block)
{
    printf("disk: direct reading %i\n", block);

    // block -> cylinder, sector
    int cylinder = block / n_sectors;
    int sector = block % n_sectors;

    // cylinder, sector -> req_str
    char req_str[20];
    snprintf(req_str, 20, "R %d %d", cylinder, sector);

    // req_str -> res_buffer
    char res_buffer[DEFAULT_BUFFER_CAPACITY];
    int res_size;
    int result = disk_server_response(0, req_str, strlen(req_str), res_buffer, &res_size, DEFAULT_BUFFER_CAPACITY);
    RET_ERR_RESULT(result);
    RET_ERR_IF(res_size != 4 + BLOCK_SIZE, , READ_ERROR);
    RET_ERR_IF(!starts_with(res_buffer, res_size, "Yes"), , READ_ERROR);

    // res_buffer -> buffer
    memcpy(buffer, res_buffer + 4, BLOCK_SIZE);
    return BLOCK_SIZE;
}

int disk_write_direct(const char buffer[BLOCK_SIZE], int block)
{
    printf("disk: direct writing %i\n", block);

    // block -> cylinder, sector
    int cylinder = block / n_sectors;
    int sector = block % n_sectors;

    // cylinder, sector -> req_head_str
    char req_head_str[30];
    snprintf(req_head_str, 30, "W %d %d %d ", cylinder, sector, BLOCK_SIZE);

    // req_head_str, buffer -> req_buffer
    char req_buffer[DEFAULT_BUFFER_CAPACITY];
    int req_size;
    int result = concat_buffer(req_buffer, &req_size, DEFAULT_BUFFER_CAPACITY, req_head_str, strlen(req_head_str), buffer, BLOCK_SIZE);
    RET_ERR_RESULT(result);

    // req_buffer -> res_buffer
    char res_buffer[DEFAULT_BUFFER_CAPACITY];
    int res_size;
    result = disk_server_response(0, req_buffer, req_size, res_buffer, &res_size, DEFAULT_BUFFER_CAPACITY);
    RET_ERR_RESULT(result);
    RET_ERR_IF(!starts_with(res_buffer, res_size, "Yes"), , READ_ERROR);
    return res_size;
}

int disk_read(char buffer[BLOCK_SIZE], int block)
{
    printf("disk: reading %i\n", block);
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
    int result;
    while (ref[victim] == 1)
    {
        ref[victim] = 0;
        victim = (victim + 1) % CACHE_SIZE;
    }
    if (ref[victim] == 0)
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
    printf("disk: writing %i\n", block);
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
    int result;
    while (ref[victim] == 1)
    {
        ref[victim] = 0;
        victim = (victim + 1) % CACHE_SIZE;
    }
    if (ref[victim] == 0)
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
