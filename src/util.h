#ifndef BMATH_UTIL_H
#define BMATH_UTIL_H

#define __repeat_character(fp, n, c)                    \
do {                                                    \
	for (int i = 0; i < n; i++) fputc(c, fp);       \
} while (0)

#define __print_hex(n, b, u)                            \
do {                                                    \
	if (u) {                                        \
		printf("%0*llX", b, n);                 \
	} else {                                        \
		printf("%0*llx", b, n);                 \
	}                                               \
} while(0)

#endif
