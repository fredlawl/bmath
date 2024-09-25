#ifndef BMATH_UTIL_H
#define BMATH_UTIL_H

#include <inttypes.h>

#define __repeat_character(fp, n, c)        \
	do {                                \
		for (int i = 0; i < n; i++) \
			fputc(c, fp);       \
	} while (0)

// TODO: This could easily be pass in a buffer or something
#define __print_hex(n, b, u)                        \
	do {                                        \
		if (u) {                            \
			printf("%0*" PRIX64, b, n); \
		} else {                            \
			printf("%0*" PRIx64, b, n); \
		}                                   \
	} while (0)

#endif
