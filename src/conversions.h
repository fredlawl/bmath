#ifndef BMATH_CONVERSIONS_H
#define BMATH_CONVERSIONS_H

#include <stdint.h>
#include <ctype.h>
#include <math.h>

#include "shim.h"

static bool str_hex_to_uint64(const char *input, uint64_t *result)
{
	char current_char;
	uint64_t exp;

	size_t input_length = strlen(input) - 1;
	const char *input_start = input;
	const char *input_end = input + input_length;
	int counter = 0;

	if (*input_start++ != '0')
		return false;

	if (*input_start != 'x' && *input_start != 'X')
		return false;

	*result = 0;
	while (true) {
		current_char = *input_end--;
		if (current_char == 'x' || current_char == 'X')
			break;

		if (!__ishexnumber(current_char))
			return false;

		exp = (uint64_t) pow(16, counter);
		counter++;

		if (isdigit(current_char)) {
			*result += (uint64_t) (current_char - '0') * exp;
			continue;
		}

		current_char = (current_char >= 'A' &&
				current_char <= 'F') ?
				current_char :
                               (char) (current_char - 32);

		*result += ((current_char - 'A') + 10) * exp;
	}

	return true;
}

#endif
