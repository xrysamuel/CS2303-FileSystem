#ifndef STRTING_PROCESS_H
#define STRTING_PROCESS_H

#include "std.h"

bool starts_with(const char *buffer, int size, const char *prefix);

int concat_buffer(char **p_target_buffer, int *p_target_size, const char *src_buffer_1, int src_size_1, const char *src_buffer_2, int src_size_2);

int buffer_to_str(const char *buffer, int size, char **p_str);

int buffer_to_hex_str(const char *buffer, int size, char **p_str);

int str_to_buffer(const char *str, char **p_buffer, int *p_size);

#endif