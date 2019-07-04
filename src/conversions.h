#ifndef BMATH_CONVERSIONS_H
#define BMATH_CONVERSIONS_H

#include <stdlib.h>
#include <inttypes.h>
#include <ctype.h>
#include <string.h>
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

static uint64_t str_to_uint64(const char *input)
{
	char current_char;
	uint64_t result = 0;
	const char *start = input;

	while ((current_char = *start++) != '\0') {
		if (!isdigit(current_char))
			continue;

		result = result * 10 + (current_char - '0');
	}

	return result;
}

static size_t convert_uint64_to_hex(uint64_t num, char **out, bool uppercase)
{
	char *writep;
	uint64_t quot = num;
	int remainder = 0;
	size_t str_len = 0;

	*out = NULL;
	while (quot > 0) {
		str_len++;
		quot /= 16;
	}

	if (num == 0)
		str_len++;

	*out = (char *) calloc(str_len + 1, sizeof(char));
	if (*out == NULL)
		return 0;

	writep = *out;
	if (num == 0) {
		*writep++ = '0';
		return str_len;
	}

	// Fill from end of string to front to avoid using a str reverse
	// function.
	writep += str_len - 1;
	quot = num;
	while (quot > 0) {
		char cnum;
		int offset;
		remainder = quot % 16;
		cnum = remainder + '0';

		if (cnum > '9') {
			offset = (remainder - 10) % 6;
			*writep-- = (uppercase) ? offset + 'A' : offset + 'a';
		} else {
			*writep-- = cnum;
		}

		quot /= 16;
	}

	return str_len;
}

#endif
