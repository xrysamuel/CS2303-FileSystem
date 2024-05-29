#ifndef DISK_H
#define DISK_H

#include "fsconfig.h"

#define CACHE_SIZE 1024

void disk_init(const char *server_ip, int port);

void disk_close();

void get_n_blocks(int *p_n_blocks);

int disk_read(char buffer[BLOCK_SIZE], int block);

int disk_write(const char buffer[BLOCK_SIZE], int block);

#endif