#pragma once

#include <inttypes.h>

#define __repeat_character(fp, n, c)           \
	do {                                   \
		for (int _i = 0; _i < n; _i++) \
			fputc(c, fp);          \
	} while (0)

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
