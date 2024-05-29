#ifndef ERROR_H
#define ERROR_H

#include "common.h"

// #define SILENCE


// error handling:
// 1. We only exit when the function responsible for handling errors has the
//    ability to perform the final cleanup. Otherwise, the error result should
//    be forwarded or reinterpreted to the caller.: RET_ERR_IF -> RET_ERR_IF ->
//    ... -> RET_ERR_IF -> (T)EXIT_IF
// 2. If the error is recoverable, when the function generating the response
//    encounters an error, it can generate an appropriate error response:
//    RET_ERR_IF -> RET_ERR_IF -> ... -> RET_ERR_IF  -> error response

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

// unrecoverable fault
#define DEFAULT_ERROR -1
#define READ_ERROR -2
#define WRITE_ERROR -3
#define INVALID_ARG_ERROR -4
#define BAD_ALLOC_ERROR -5
#define BUFFER_OVERFLOW -6

// specific errors
#define DISK_FULL_ERROR -7
#define PERMISSION_DENIED -8
#define NOT_FOUND -9

#define IS_ERROR(result) result < 0

#define RET_ERR_RESULT(result) RET_ERR_IF(IS_ERROR(result), , result);

#endif