#pragma once

#include <stdint.h>
#include <unistd.h>

ssize_t str_hex_to_uint64(char *input, ssize_t input_length, uint64_t *result);
