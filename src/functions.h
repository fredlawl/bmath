#pragma once

#include <stdint.h>

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
typedef enum func_err (*bmath_func_t)(uint64_t *, int,
				      uint64_t argv[FUNCTIONS_MAX_OPS]);

enum func_err align(uint64_t *retval, int argc,
		    uint64_t argv[FUNCTIONS_MAX_OPS]);
enum func_err align_down(uint64_t *retval, int argc,
			 uint64_t argv[FUNCTIONS_MAX_OPS]);
enum func_err mask(uint64_t *retval, int argc,
		   uint64_t argv[FUNCTIONS_MAX_OPS]);
enum func_err popcnt(uint64_t *retval, int argc,
		     uint64_t argv[FUNCTIONS_MAX_OPS]);
enum func_err bswap(uint64_t *retval, int argc,
		    uint64_t argv[FUNCTIONS_MAX_OPS]);
