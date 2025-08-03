#include <assert.h>
#include <criterion/alloc.h>
#include <criterion/criterion.h>
#include <criterion/internal/assert.h>
#include <criterion/logging.h>
#include <criterion/parameterized.h>
#include <inttypes.h>
#include <stdint.h>

#include "../src/functions.h"

struct func_params {
	uint64_t expected;
	enum func_err err;
	int argc;
	uint64_t argv[FUNCTIONS_MAX_OPS];
};

static void setup(void)
{
}

static void teardown(void)
{
}

static void check(const char *test_func, struct func_params *param,
		  bmath_func_t func)
{
	uint64_t ret = 0;
	enum func_err err;

	err = func(&ret, param->argc, param->argv);
	cr_assert_eq(err, param->err, "%s: ret == %d", test_func, err);
	cr_assert_eq(ret, param->expected, "%s: expected == %lu", test_func,
		     ret);
}

TestSuite(functions, .init = setup, .fini = teardown);

ParameterizedTestParameters(functions, align_test)
{
	static struct func_params params[] = { { 0, FUNC_EINVAL, 0, { 0 } },
					       { 0, FUNC_EINVAL, 1, { 0 } },
					       { 0, FUNC_ESUCCESS, 2, { 0 } } };
	size_t nparams = sizeof(params) / sizeof(params[0]);
	return cr_make_param_array(struct func_params, params, nparams);
}

ParameterizedTest(struct func_params *param, functions, align_test)
{
	check(__func__, param, align);
}

ParameterizedTestParameters(functions, align_down_test)
{
	static struct func_params params[] = { { 0, FUNC_EINVAL, 0, { 0 } },
					       { 0, FUNC_EINVAL, 1, { 0 } },
					       { 0, FUNC_ESUCCESS, 2, { 0 } } };
	size_t nparams = sizeof(params) / sizeof(params[0]);
	return cr_make_param_array(struct func_params, params, nparams);
}

ParameterizedTest(struct func_params *param, functions, align_down_test)
{
	check(__func__, param, align_down);
}

ParameterizedTestParameters(functions, mask_test)
{
	static struct func_params params[] = { { 0, FUNC_ESUCCESS, 0, { 0 } },
					       { 0, FUNC_ESUCCESS, 1, { 0 } },
					       { 0, FUNC_ERANGE, 1, { 9 } },
					       { 0, FUNC_ERANGE, 1, { -1 } },
					       { 0, FUNC_EINVAL, 2, { 0 } } };
	size_t nparams = sizeof(params) / sizeof(params[0]);
	return cr_make_param_array(struct func_params, params, nparams);
}

ParameterizedTest(struct func_params *param, functions, mask_test)
{
	check(__func__, param, mask);
}

ParameterizedTestParameters(functions, popcnt_test)
{
	static struct func_params params[] = {
		{ 0, FUNC_EINVAL, 0, { 0 } },
		{ 0, FUNC_ESUCCESS, 1, { 0 } },
	};
	size_t nparams = sizeof(params) / sizeof(params[0]);
	return cr_make_param_array(struct func_params, params, nparams);
}

ParameterizedTest(struct func_params *param, functions, popcnt_test)
{
	check(__func__, param, popcnt);
}

ParameterizedTestParameters(functions, bswap_test)
{
	static struct func_params params[] = {
		{ 0, FUNC_EINVAL, 0, { 0 } },
		{ 0, FUNC_ESUCCESS, 1, { 0 } },
	};
	size_t nparams = sizeof(params) / sizeof(params[0]);
	return cr_make_param_array(struct func_params, params, nparams);
}

ParameterizedTest(struct func_params *param, functions, bswap_test)
{
	check(__func__, param, bswap);
}
