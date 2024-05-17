#include "disk.h"
#include "std.h"
#include "error_type.h"
#include "client.h"
#include "buffer.h"
#include "definition.h"

response_t disk_server_response;
int n_cylinder;
int n_sectors;

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
}

void disk_close()
{
    custom_client_close();
}


void get_n_blocks(int *p_n_blocks)
{
    *p_n_blocks = n_cylinder * n_sectors;
}

int disk_read_to(char buffer[BLOCK_SIZE], int block)
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
    RET_ERR_IF(IS_ERROR(result), , result);
    RET_ERR_IF(res_size != 4 + BLOCK_SIZE, , READ_ERROR);
    RET_ERR_IF(!starts_with(res_buffer, res_size, "Yes"), , READ_ERROR);

    // res_buffer -> buffer
    memcpy(buffer, res_buffer + 4, BLOCK_SIZE);
    free(res_buffer);
    return BLOCK_SIZE;
}

int disk_write_from(char buffer[BLOCK_SIZE], int block)
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
    RET_ERR_IF(IS_ERROR(result), , result);

    // req_buffer -> res_buffer
    char *res_buffer = NULL;
    int res_size;
    int result = disk_server_response(req_buffer, req_size, &res_buffer, &res_size);
    free(req_buffer);
    RET_ERR_IF(IS_ERROR(result), , result);
    RET_ERR_IF(!starts_with(res_buffer, res_size, "Yes"), , READ_ERROR);
    return res_size;
}