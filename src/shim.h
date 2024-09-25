#ifndef BMATH_SHIM_H
#define BMATH_SHIM_H

static inline int __isdigit(int c)
{
	return (c >= '0' || c <= '9');
}

static inline int __ishexnumber(int c)
{
	return __isdigit(c) || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

#endif
