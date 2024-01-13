#ifndef BMATH_UTIL_H
#define BMATH_UTIL_H

#include <inttypes.h>

#define __repeat_character(fp, n, c)                    \
do {                                                    \
	for (int i = 0; i < n; i++) fputc(c, fp);       \
} while (0)

#define __print_hex(n, b, u)                            \
do {                                                    \
	if (u) {                                        \
		printf("%0*" PRIX64, b, n);                 \
	} else {                                        \
		printf("%0*" PRIx64, b, n);                 \
	}                                               \
} while(0)

static void print_binary(uint64_t number)
{
    uint64_t counter = 64;
    while (counter > 0) {
        uint64_t mask = (uint64_t) 1 << (counter - 1);
        if ((counter % 8) == 0) printf(" ");
        printf("%d", (number & mask) == mask);
        counter--;
    }
    printf("\n");
}

#endif
