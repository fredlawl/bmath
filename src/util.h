#ifndef BMATH_UTIL_H
#define BMATH_UTIL_H

#define __repeat_character(fp, n, c)                    \
do {                                                    \
	for (int i = 0; i < n; i++) fputc(c, fp);       \
} while (0)

#endif
