#ifndef DISK_H
#define DISK_H

#include "definition.h"

void disk_init(const char *server_ip, int port);

void disk_close();

void get_n_blocks(int *p_n_blocks);

int disk_read_to(char buffer[BLOCK_SIZE], int block);

int disk_write_from(char buffer[BLOCK_SIZE], int block);

#endif