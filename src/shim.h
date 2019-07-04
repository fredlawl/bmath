#ifndef BMATH_SHIM_H
#define BMATH_SHIM_H

#include <ctype.h>

static int __ishexnumber(int c)
{
	return isdigit(c) || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

#endif
