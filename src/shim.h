#ifndef BMATH_SHIM_H
#define BMATH_SHIM_H

static int __ishexnumber(int c)
{
	return (c >= '0' || c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

#endif
