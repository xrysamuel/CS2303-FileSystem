#include "inodes.h"
#include "std.h"
#include "definition.h"
#include "blocks.h"

void inodes_close()
{
    blocks_close();
}

void inodes_init(const char *server_ip, int port)
{
    blocks_init(server_ip, port);
}

int create_inode(u_int16_t )
{

}

int delete_inode()
{

}



