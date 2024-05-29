#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <arpa/inet.h>   
#include <sys/wait.h>
#include <stdbool.h>
#include <signal.h>
#include <pthread.h>
#include <sys/time.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <ctype.h>

#define DEFAULT_BUFFER_CAPACITY 65536

#endif