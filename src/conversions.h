#pragma once

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

size_t str_hex_to_uint64(const char *input, ssize_t input_length,
			 uint64_t *result);
