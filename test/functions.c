#define UNITY_SUPPORT_TEST_CASES

#include <inttypes.h>
#include <stdint.h>
#include <unity/unity.h>

#include "../src/functions.h"

struct func_params {
	const char *name;
	uint64_t expected;
	enum func_err err;
	int argc;
	uint64_t argv[FUNCTIONS_MAX_OPS];
};

static void check(struct func_params *param, bmath_func_t func)
{
	char call_msg[256];
	char out_msg[256];
	uint64_t out_value = 0;
	enum func_err ret;

	sprintf(call_msg, "%s: ret is expected", param->name);
	sprintf(out_msg, "%s: result is expected", param->name);

	ret = func(&out_value, param->argc, param->argv);
	TEST_ASSERT_EQUAL_MESSAGE(param->err, ret, call_msg);
	TEST_ASSERT_EQUAL_MESSAGE(param->expected, out_value, out_msg);
}

void setUp(void)
{
}

void tearDown(void)
{
}

void test_align()
{
	struct func_params params[] = {
		{ "no args", 0, FUNC_EINVAL, 0, { 0 } },
		{ "one arg", 0, FUNC_EINVAL, 1, { 0 } },
		{ "correct args", 0, FUNC_ESUCCESS, 2, { 0 } },
	};

	for (size_t i = 0; i < sizeof(params) / sizeof(params[0]); i++) {
		check(&params[i], align);
	}
}

void test_align_down()
{
	struct func_params params[] = {
		{ "no args", 0, FUNC_EINVAL, 0, { 0 } },
		{ "one arg", 0, FUNC_EINVAL, 1, { 0 } },
		{ "correct args", 0, FUNC_ESUCCESS, 2, { 0 } }
	};

	for (size_t i = 0; i < sizeof(params) / sizeof(params[0]); i++) {
		check(&params[i], align_down);
	}
}

void test_mask()
{
	struct func_params params[] = {
		{ "no args", 0, FUNC_ESUCCESS, 0, { 0 } },
		{ "one arg", 0, FUNC_ESUCCESS, 1, { 0 } },
		{ "over arg 8", 0, FUNC_ERANGE, 1, { 9 } },
		{ "less arg 0", 0, FUNC_ERANGE, 1, { -1 } },
		{ "multiple args", 0, FUNC_EINVAL, 2, { 0 } }
	};

	for (size_t i = 0; i < sizeof(params) / sizeof(params[0]); i++) {
		check(&params[i], mask);
	}
}

void test_popcnt()
{
	struct func_params params[] = {
		{ "no args", 0, FUNC_EINVAL, 0, { 0 } },
		{ "one arg", 0, FUNC_ESUCCESS, 1, { 0 } }
	};

	for (size_t i = 0; i < sizeof(params) / sizeof(params[0]); i++) {
		check(&params[i], popcnt);
	}
}

void test_bswap()
{
	struct func_params params[] = {
		{ "no args", 0, FUNC_EINVAL, 0, { 0 } },
		{ "one arg", 0, FUNC_ESUCCESS, 1, { 0 } },
	};

	for (size_t i = 0; i < sizeof(params) / sizeof(params[0]); i++) {
		check(&params[i], popcnt);
	}
}

int main(void)
{
	UNITY_BEGIN();
	RUN_TEST(test_align);
	RUN_TEST(test_align_down);
	RUN_TEST(test_mask);
	RUN_TEST(test_popcnt);
	RUN_TEST(test_bswap);
	return UNITY_END();
}
