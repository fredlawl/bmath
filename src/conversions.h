#ifndef BMATH_CONVERSIONS_H
#define BMATH_CONVERSIONS_H

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include "shim.h"

static size_t str_hex_to_uint64(const char *input, ssize_t input_length,
				uint64_t *result)
{
	char current_char;
	ssize_t bytes_parsed = 0;

	const char *input_start = input;

	if (*input_start++ != '0') {
		errno = EINVAL;
		return bytes_parsed;
	}

	if (!(*input_start == 'x' || *input_start == 'X')) {
		errno = EINVAL;
		return bytes_parsed;
	}

	input_start++;
	bytes_parsed += 2;

	*result = 0;
	do {
		uint64_t temp;
		current_char = *input_start++;

		if (!__ishexnumber(current_char))
			return bytes_parsed;

		*result = *result << 4;
		if (__isdigit(current_char)) {
			temp = current_char - '0';
		} else {
			current_char =
				(current_char >= 'A' && current_char <= 'F') ?
					current_char :
					(char)(current_char - 32);
			temp = (current_char - 'A') + 10;
		}

		*result = *result + temp;
		bytes_parsed++;

		if (bytes_parsed > input_length) {
			errno = EOVERFLOW;
			return 0;
		}

	} while (1);

	return bytes_parsed;
}

#endif
