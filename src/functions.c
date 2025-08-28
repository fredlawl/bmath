#include <stdint.h>

#include "functions.h"

enum func_err align(uint64_t *ret, int argc, uint64_t argv[FUNCTIONS_MAX_OPS])
{
	*ret = 0;
	if (argc != 2) {
		return FUNC_EINVAL;
	}
	*ret = ((uint64_t)argv[0] + (uint64_t)argv[1] - 1) &
	       ~((uint64_t)argv[1] - 1);
	return 0;
}

enum func_err align_down(uint64_t *ret, int argc,
			 uint64_t argv[FUNCTIONS_MAX_OPS])
{
	*ret = 0;
	if (argc != 2) {
		return FUNC_EINVAL;
	}
	*ret = (uint64_t)argv[0] & ~((uint64_t)argv[1] - 1);
	return 0;
}

enum func_err bswap(uint64_t *ret, int argc, uint64_t argv[FUNCTIONS_MAX_OPS])
{
	*ret = 0;

	if (argc != 1) {
		return FUNC_EINVAL;
	}

	if (argv[0] > UINT32_MAX) {
		*ret = (uint64_t)__builtin_bswap64(argv[0]);
		return 0;
	}

	if (argv[0] > UINT16_MAX) {
		*ret = (uint64_t)__builtin_bswap32((uint32_t)argv[0]);
		return 0;
	}

	if (argv[0] > UINT8_MAX) {
		*ret = (uint64_t)__builtin_bswap16((uint16_t)argv[0]);
		return 0;
	}

	*ret = argv[0];
	return 0;
}

enum func_err clz(uint64_t *ret, int argc, uint64_t argv[FUNCTIONS_MAX_OPS])
{
	uint64_t zeros, expected_zeros, actual_zeros;
	uint64_t value, bytes;
	*ret = 0;

	if (argc != 2) {
		return FUNC_EINVAL;
	}

	value = argv[0];
	bytes = argv[1];

	if (bytes <= 0 || bytes > 8) {
		return FUNC_ERANGE;
	}

	if (value == 0) {
		return 0;
	}

	zeros = __builtin_clzll(value);
	expected_zeros = bytes * 8;
	actual_zeros = zeros - ((8 - bytes) * 8);

	if (actual_zeros >= expected_zeros) {
		return FUNC_ERANGE;
	}

	*ret = actual_zeros;
	return 0;
}

enum func_err ctz(uint64_t *ret, int argc, uint64_t argv[FUNCTIONS_MAX_OPS])
{
	*ret = 0;

	if (argc != 1) {
		return FUNC_EINVAL;
	}

	if (argv[0] == 0) {
		return 0;
	}

	*ret = __builtin_ctzll(argv[0]);
	return 0;
}

enum func_err mask(uint64_t *ret, int argc, uint64_t argv[FUNCTIONS_MAX_OPS])
{
	*ret = 0;
	if (argc == 0) {
		*ret = 0;
		return 0;
	}

	if (argc != 1) {
		return FUNC_EINVAL;
	}

	if (argv[0] < 0 || argv[0] > 8) {
		return FUNC_ERANGE;
	}

	if (argv[0] == 8) {
		*ret = ~0;
		return 0;
	}

	*ret = ~((uint64_t)~0 << (argv[0] * (uint64_t)8));
	return 0;
}

enum func_err popcnt(uint64_t *ret, int argc, uint64_t argv[FUNCTIONS_MAX_OPS])
{
	*ret = 0;

	if (argc != 1) {
		return FUNC_EINVAL;
	}

	*ret = (uint64_t)__builtin_popcountll(argv[0]);
	return 0;
}
