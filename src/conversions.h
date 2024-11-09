#ifndef BMATH_CONVERSIONS_H
#define BMATH_CONVERSIONS_H

#include <ctype.h>
#include <stdint.h>
#include <unistd.h>

#include "shim.h"

static bool str_hex_to_uint64(const char *input, size_t input_length,
			      uint64_t *result)
{
	char current_char;

	const char *input_start = input;
	const char *input_end = input + input_length;

	if (*input_start++ != '0')
		return false;

	if (!(*input_start == 'x' || *input_start == 'X'))
		return false;

	input_start++;

	*result = 0;
	do {
		uint64_t temp;
		current_char = *input_start++;

		if (!__ishexnumber(current_char))
			return false;

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
	} while (input_start != input_end);

	return true;
}

#endif
