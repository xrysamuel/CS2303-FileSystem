#ifndef BLOCKS_H
#define BLOCKS_H

void blocks_close();

int blocks_format();

void blocks_init(const char *server_ip, int port);

int deallocate_inode(int inode_id);

int allocate_inode(int *inode_id);

int read_inode(int inode_id, struct inode_t *inode);

int write_inode(int inode_id, struct inode_t inode);

int deallocate_block(int block_id);

int allocate_block(int *block_id);

int read_block(int block_id, char block[256]);

int write_block(int block_id, char block[256]);

#endif