#pragma once

#include <stdint.h>

typedef uint64_t bmath_result_t;

static inline bmath_result_t bmath_result_t__add(bmath_result_t a,
						 bmath_result_t b)
{
	return a + b;
}

static inline bmath_result_t bmath_result_t__sub(bmath_result_t a,
						 bmath_result_t b)
{
	return a - b;
}

static inline bmath_result_t bmath_result_t__mul(bmath_result_t a,
						 bmath_result_t b)
{
	return a * b;
}

static inline bmath_result_t bmath_result_t__div(bmath_result_t a,
						 bmath_result_t b)
{
	return a / b;
}

static inline bmath_result_t bmath_result_t__mod(bmath_result_t a,
						 bmath_result_t b)
{
	return a % b;
}

static inline bmath_result_t bmath_result_t__lshift(bmath_result_t a,
						    bmath_result_t b)
{
	return a << b;
}

static inline bmath_result_t bmath_result_t__rshift(bmath_result_t a,
						    bmath_result_t b)
{
	return a >> b;
}

static inline bmath_result_t bmath_result_t__and(bmath_result_t a,
						 bmath_result_t b)
{
	return a & b;
}

static inline bmath_result_t bmath_result_t__or(bmath_result_t a,
						bmath_result_t b)
{
	return a | b;
}

static inline bmath_result_t bmath_result_t__xor(bmath_result_t a,
						 bmath_result_t b)
{
	return a ^ b;
}

static inline bmath_result_t bmath_result_t__complement(bmath_result_t a)
{
	return ~a;
}

static inline bmath_result_t bmath_result_t__negate(bmath_result_t a)
{
	return -a;
}

static inline uint64_t bmath_result_t__to_uint64_t(bmath_result_t ret)
{
	return (uint64_t)ret;
}

static inline bmath_result_t bmath_result_t__from_uint64(uint64_t ret)
{
	return (bmath_result_t)ret;
}
