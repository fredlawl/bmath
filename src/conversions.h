#ifndef BMATH_CONVERSIONS_H
#define BMATH_CONVERSIONS_H

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include "lookup_tables.h"

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

	switch (*input_start) {
	case 'x':
	case 'X':
		break;
	default:
		errno = EINVAL;
		return bytes_parsed;
	}

	input_start++;
	bytes_parsed += 2;

	*result = 0;
	do {
		current_char = *input_start++;

		if (!__is_allowed_hex(current_char))
			return bytes_parsed;

		*result = (*result << 4) + __hex_to_value(current_char);

	} while (bytes_parsed++ < input_length);

	if (bytes_parsed > input_length) {
		errno = E2BIG;
		return 0;
	}

	return bytes_parsed;
}

#endif
