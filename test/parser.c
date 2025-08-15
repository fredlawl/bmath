#include <stdio.h>
#include <string.h>
#include <unity/unity.h>

#include "../src/parser.h"

struct expr_expected_params {
	char *expression;
	uint64_t expected;
};

struct expr_expected_err_params {
	char *expression;
	uint64_t expected;
	int err;
};

static struct parser_settings pctx_settings;
static struct parser_context *pctx;

static void check(const struct expr_expected_err_params *param)
{
	char ret_msg[256];
	char out_msg[256];
	uint64_t actual = 0;
	size_t exprlen = strlen(param->expression);
	int ret = parse(pctx, param->expression, exprlen, &actual);

	sprintf(ret_msg, "ret is expected for \"%s\"", param->expression);
	sprintf(out_msg, "out is expected for \"%s\"", param->expression);

	TEST_ASSERT_EQUAL_MESSAGE(param->err, ret, ret_msg);

	if (param->err) {
		return;
	}

	TEST_ASSERT_EQUAL_MESSAGE(param->expected, actual, out_msg);
}

static inline void check_basic(const struct expr_expected_params *param)
{
	check(&(struct expr_expected_err_params){ param->expression,
						  param->expected, 0 });
}

void setUp(void)
{
	FILE *dev_null;

	pctx_settings = (struct parser_settings){ .max_parse_len = 128, NULL };

	dev_null = fopen("/dev/null", "w");
	if (ferror(dev_null)) {
		TEST_FAIL_MESSAGE("unable to open /dev/null");
		return;
	}

	pctx_settings.err_stream = dev_null;
	pctx = parser_new(&pctx_settings);
}

void tearDown(void)
{
	if (pctx_settings.err_stream) {
		fclose(pctx_settings.err_stream);
	}
	parser_free(pctx);
}

void test_basic_validation()
{
	struct expr_expected_err_params params[] = {
		{ "", 0, PE_NOTHING_TO_PARSE },
		{ "QdZeQU3D2gu4o3udqRxpVhIaxSS43AIzVKS9RN7zrsjNf3A6MwjLADgqU9tBUqa7xmvgXrZMdSaeVMOv2f3f70EPwvKkjdbqIhc5r73hJcEN5azTJIRX8f8EN5X_OVER_LIMIT",
		  0, PE_EXPRESSION_TOO_LONG },
		{ ".!@#$`.,<>?/\\;:'\"[]{}=_", 0, PE_PARSE_ERROR },
		{ "\t\n    1 ", 1, 0 },
		{ "1.0", 0, PE_PARSE_ERROR },
		{ "1 || 3", 0, PE_PARSE_ERROR },
		{ "2 % 0", 0, PE_PARSE_ERROR },
	};

	for (size_t i = 0; i < sizeof(params) / sizeof(params[0]); i++) {
		check(&params[i]);
	}
}

void test_factors()
{
	struct expr_expected_err_params params[] = {
		// parsing hex assumes 0 results by default, this was because of optmizations
		{ "0x", 0, 0 },	 { "(1)", 1, 0 },    { "1", 1, 0 },
		{ "0x1", 1, 0 }, { "(0xa)", 10, 0 }, { "~(0)", ~0, 0 },
	};

	for (size_t i = 0; i < sizeof(params) / sizeof(params[0]); i++) {
		check(&params[i]);
	}
}

void test_ops()
{
	struct expr_expected_params params[] = {
		{ "1 | 2", 1 | 2 }, { "2 & 2", 2 & 2 },	  { "1 & 2", 1 & 2 },
		{ "3 ^ 2", 3 ^ 2 }, { "1 << 3", 1 << 3 }, { "8 >> 1", 8 >> 1 },
		{ "~0", ~0 },	    { "2 * 1", 2 * 1 },	  { "2 % 2", 2 % 2 },
		{ "2 % 1", 2 % 1 }
	};

	for (size_t i = 0; i < sizeof(params) / sizeof(params[0]); i++) {
		check_basic(&params[i]);
	}
}

void test_term2()
{
	struct expr_expected_err_params params[] = {
		{ "1+1", 2, 0 },
		{ "1-1", 0, 0 },
	};

	for (size_t i = 0; i < sizeof(params) / sizeof(params[0]); i++) {
		check(&params[i]);
	}
}

void test_signed_factors()
{
	struct expr_expected_err_params params[] = {
		{ "0", 0, 0 },
		{ "+0", 0, 0 },
		{ "-0", 0, 0 },
		{ "1", 1, 0 },
		{ "+1", 1, 0 },
		{ "-1", -1, 0 },
		{ "+-1", +-1, 0 },
		{ "-+1", -+1, 0 },
		{ "++16", 16, 0 },
		{ "--16", 16, 0 },
		{ "---16", -16, 0 },
		{ "~-16", ~-16, 0 },
		{ "+~16", +~16, 0 },
		{ "~+16", ~+16, 0 },
		{ "-~16", -~16, 0 },
		{ "1+-1", 1 + -1, 0 },
		{ "1+-~1", 1 + -~1, 0 },
		{ "-1-1", -2, 0 },
		{ "-1-1-1", -3, 0 },

		// let this one slide. this tool has no concept of ++ as a thing
		{ "1++1", 1 + 1, 0 },
		// max depth
		{ "~~~~~~~~~~~16", 0, PE_PARSE_ERROR },
	};

	for (size_t i = 0; i < sizeof(params) / sizeof(params[0]); i++) {
		check(&params[i]);
	}
}

void test_order_of_operations()
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wparentheses"
	struct expr_expected_params params[] = {
		{ "1 & 2 ^ 3 | 4", 1 & 2 ^ 3 | 4 },
		{ "1 | 2 ^ 3 & 4", 1 | 2 ^ 3 & 4 },
		{ "1 ^ 2 | 3 & 4", 1 ^ 2 | 3 & 4 },
		{ "1 ^ 2 & 3 | 4", 1 ^ 2 & 3 | 4 },
		{ "1 & 2 | 3 ^ 4", 1 & 2 | 3 ^ 4 },
		{ "1 | 2 & 3 ^ 4", 1 | 2 & 3 ^ 4 },
		{ "1 | 2 & 3 ^ 4 << 1", 1 | 2 & 3 ^ 4 << 1 },
		{ "1 | 2 & 3 ^ 4 >> 1", 1 | 2 & 3 ^ 4 >> 1 },
		{ "1 | 2 & 3 ^ 4 >> 1 + 5", 1 | 2 & 3 ^ 4 >> 1 + 5 },
		{ "1 + 5 | 2 & 3 ^ 4 << 1", 1 + 5 | 2 & 3 ^ 4 << 1 },
		{ "1 - 5 | 2 & 3 ^ 4 << 1", 1 - 5 | 2 & 3 ^ 4 << 1 },
		{ "2 * 1 - 5 | 2 & 3 ^ 4 << 1", 2 * 1 - 5 | 2 & 3 ^ 4 << 1 },
		{ "2 * 1 - 5 | 2 & 3 ^ 4 << 1 % 2",
		  2 * 1 - 5 | 2 & 3 ^ 4 << 1 % 2 },
		{ "1 & 2 & 3 & 4", 1 & 2 & 3 & 4 },
		{ "1 | 2 | 3 | 4", 1 | 2 | 3 | 4 },
		{ "1 ^ 2 ^ 3 ^ 4", 1 ^ 2 ^ 3 ^ 4 },
		{ "1 + 2 + 3 + 4", 1 + 2 + 3 + 4 },
		{ "1 - 2 - 3 - 4", 1 - 2 - 3 - 4 },
		{ "1 * 2 * 3 * 4", 1 * 2 * 3 * 4 },
		{ "1 % 2 % 3 % 4", 1 % 2 % 3 % 4 },
	};
#pragma GCC diagnostic pop

	for (size_t i = 0; i < sizeof(params) / sizeof(params[0]); i++) {
		check_basic(&params[i]);
	}
}

// TODO: Should reconsider a design such that expect next token is mocked
// such that the next token is next in a list. That's a better way to test,
// than doing all these parses.
void test_functions()
{
	struct expr_expected_err_params params[] = {
		{ "a()", 0, PE_PARSE_ERROR },
		{ "align)", 0, PE_PARSE_ERROR },
		{ "align(", 0, PE_PARSE_ERROR },
		{ "align(7,8", 0, PE_PARSE_ERROR },
		{ "align(7,)", 0, PE_PARSE_ERROR },
		{ "align(7,8)", 8, 0 },
		{ "22+align(7,8)+22", 8 + 22 * 2, 0 },
		{ "align(7,1<<3)", 8, 0 },
		{ "align(1<<3-1,1<<3)", 8, 0 },
		{ "align(mask(2),1<<3)", 65536, 0 },
		{ "align(7,8,9)", 0, PE_PARSE_ERROR },
		{ "align_down(7,8)", 0, 0 },
		{ "mask(2)", 0xffff, 0 },
		{ "mask()", 0, 0 },
		{ "bswap(0xabcd)", 0xcdab, 0 },
		{ "popcnt(3)", 2, 0 },
	};

	for (size_t i = 0; i < sizeof(params) / sizeof(params[0]); i++) {
		check(&params[i]);
	}
}

void test_concat_expressions()
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wparentheses"
	struct expr_expected_params params[] = {
		{ "1 | 3 << 1 | 3", 1 | 3 << 1 | 3 },
		{ "(1 << 0) | (1 << 1)", (1 << 0) | (1 << 1) },
		{ "0x1 | 0x2 | 0x4 | 0x8", 0x1 | 0x2 | 0x4 | 0x8 },
		{ "(1 << 0) | (1 << 1) | (1 << 2) | (1 << 3)",
		  (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) },
		{ "(((1 << 0) | (1 << 1)) | (1 << 2)) | (1 << 3)",
		  (((1 << 0) | (1 << 1)) | (1 << 2)) | (1 << 3) },
		{ "(1 << 0) & 0x01", (1 << 0) & 0x01 },
		{ "0x01 & (1 << 0) & 0x0", 0x01 & (1 << 0) & 0x0 },
		{ "1 << 1 << 0", 1 << 1 << 0 },
		{ "(67044172665631171 & 0x93 - 5519551265853101 % 15351520200696815) | (41514017200852255 & (473435211))",
		  (67044172665631171 &
		   0x93 - 5519551265853101 % 15351520200696815) |
			  (41514017200852255 & (473435211)) },
	};
#pragma GCC diagnostic pop

	for (size_t i = 0; i < sizeof(params) / sizeof(params[0]); i++) {
		check_basic(&params[i]);
	}
}

int main(void)
{
	UNITY_BEGIN();
	RUN_TEST(test_basic_validation);
	RUN_TEST(test_factors);
	RUN_TEST(test_ops);
	RUN_TEST(test_term2);
	RUN_TEST(test_signed_factors);
	RUN_TEST(test_order_of_operations);
	RUN_TEST(test_functions);
	RUN_TEST(test_concat_expressions);
	return UNITY_END();
}
