#ifndef STRTING_PROCESS_H
#define STRTING_PROCESS_H

#include "common.h"

bool starts_with(const char *buffer, int size, const char *prefix);

int concat_buffer(char *target_buffer, int *p_target_size, int max_target_size, const char *src_buffer_1, int src_size_1, const char *src_buffer_2, int src_size_2);

int buffer_to_str(const char *buffer, int size, char *str, int max_str_size);

int buffer_to_hex_str(const char *buffer, int size, char *str, int max_str_size);

int str_to_buffer(const char *str, char *buffer, int *p_size, int max_size);

#endif