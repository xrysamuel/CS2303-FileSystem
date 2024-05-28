#ifndef ERROR_H
#define ERROR_H

#include "std.h"

// #define SILENCE

#ifndef SILENCE
// Returns the specified return value if the given condition is true, performs necessary cleanup actions.
#define RET_ERR_IF(condition, cleanup, returnval)   \
    if (condition) {                                \
        cleanup;                                    \
        printf("%s(%d): return %d\n",               \
         __FILE__, __LINE__, returnval);            \
        return returnval;                           \
    }

// Exits the program with a failure status if the given condition is true,performs necessary cleanup actions, and prints a formatted error message
#define EXIT_IF(condition, cleanup, format, ...)        \
    do                                                  \
    {                                                   \
        if (condition) {                                \
            cleanup;                                    \
            fprintf(stderr, format, ##__VA_ARGS__);     \
            fprintf(stderr, "%s(%d): %s\n",             \
            __FILE__, __LINE__, strerror(errno));       \
            exit(EXIT_FAILURE);                         \
        }                                               \
    }                                                   \
    while (0)

// Terminates the current thread if the given condition is true, performs necessary cleanup actions, and prints a formatted error message
#define TEXIT_IF(condition, cleanup, format, ...)       \
    do                                                  \
    {                                                   \
        if (condition) {                                \
            cleanup;                                    \
            fprintf(stderr, format, ##__VA_ARGS__);     \
            fprintf(stderr, "%s(%d): %s\n",             \
            __FILE__, __LINE__, strerror(errno));       \
            pthread_exit(NULL);                         \
        }                                               \
    }                                                   \
    while (0)

#else
#define RET_ERR_IF(condition, cleanup, returnval)   \
    if (condition) {                                \
        cleanup;                                    \
        return returnval;                           \
    }
#define EXIT_IF(condition, cleanup, format, ...)        \
    do                                                  \
    {                                                   \
        if (condition) {                                \
            cleanup;                                    \
            fprintf(stderr, format, ##__VA_ARGS__);     \
            exit(EXIT_FAILURE);                         \
        }                                               \
    }                                                   \
    while (0)

#define TEXIT_IF(condition, cleanup, format, ...)       \
    do                                                  \
    {                                                   \
        if (condition) {                                \
            cleanup;                                    \
            fprintf(stderr, format, ##__VA_ARGS__);     \
            pthread_exit(NULL);                         \
        }                                               \
    }                                                   \
    while (0)
#endif

#define SUCCESS 0

// general errors
#define DEFAULT_ERROR -1
#define READ_ERROR -2
#define WRITE_ERROR -3
#define INVALID_ARG_ERROR -4
#define BAD_ALLOC_ERROR -5

// specific errors
#define DISK_FULL_ERROR -6
#define PERMISSION_DENIED -7

#define IS_ERROR(result) result < 0

#define RET_ERR_RESULT(result) RET_ERR_IF(IS_ERROR(result), , result);

#endif