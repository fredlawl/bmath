#pragma once

#include <stddef.h>
#include <stdint.h>

#include "type.h"

enum func_err { FUNC_ESUCCESS = 0, FUNC_EINVAL = 1, FUNC_ERANGE };
static const char *str_func_err_tbl[] = {
	[FUNC_ESUCCESS] = "",
	[FUNC_EINVAL] = "invalid number of arguments",
	[FUNC_ERANGE] = "argument outside of range"
};

static inline const char *str_func_err(enum func_err err)
{
	return str_func_err_tbl[err];
}

#define FUNCTIONS_MAX_OPS 7
typedef enum func_err (*bmath_func_t)(bmath_result_t *, int,
				      bmath_result_t argv[FUNCTIONS_MAX_OPS]);

enum func_err align(bmath_result_t *retval, int argc,
		    bmath_result_t argv[FUNCTIONS_MAX_OPS]);
enum func_err align_down(bmath_result_t *retval, int argc,
			 bmath_result_t argv[FUNCTIONS_MAX_OPS]);
enum func_err bswap(bmath_result_t *retval, int argc,
		    bmath_result_t argv[FUNCTIONS_MAX_OPS]);
enum func_err clz(bmath_result_t *retval, int argc,
		  bmath_result_t argv[FUNCTIONS_MAX_OPS]);
enum func_err ctz(bmath_result_t *retval, int argc,
		  bmath_result_t argv[FUNCTIONS_MAX_OPS]);
enum func_err mask(bmath_result_t *retval, int argc,
		   bmath_result_t argv[FUNCTIONS_MAX_OPS]);
enum func_err popcnt(bmath_result_t *retval, int argc,
		     bmath_result_t argv[FUNCTIONS_MAX_OPS]);
