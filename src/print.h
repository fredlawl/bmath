#pragma once

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

enum encoding_t {
	ENC_NONE = 0,
	ENC_ASCII = (1 << 0),
	ENC_UTF8 = (1 << 1),
	ENC_UTF16 = (1 << 2),
	ENC_UTF32 = (1 << 4)
};

#define ENC_INDEX(v) (1 >> v)

#define ENC_UTF (ENC_UTF8 | ENC_UTF16 | ENC_UTF32)
#define ENC_ALL (ENC_ASCII | ENC_UTF)

#define __print_hex(n, b, u)        \
	do {                        \
		print_hex(u, b, n); \
	} while (0)

void print_set_stream(FILE *);
void print_hex(bool, int, uint64_t);
void print_binary(uint64_t number);
void print_number(uint64_t num, bool uppercase_hex, int encoding_mask);
