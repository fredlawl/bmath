#include <criterion/criterion.h>
#include <inttypes.h>

#include "../src/conversions.h"

static void setup(void)
{

}

static void teardown(void)
{

}

Test(conversions, verify_string_is_parsed_as_uint64, .init = setup, .fini =
	teardown, .exit_code = EXIT_SUCCESS)
{
	uint64_t expected = 120;
	uint64_t actual = str_to_uint64("120");
	cr_assert_eq(actual, expected);
}

Test(conversions, verify_number_is_extracted_from_string, .init = setup,
	.fini = teardown, .exit_code = EXIT_SUCCESS)
{
	uint64_t expected = 120;
	uint64_t actual = str_to_uint64("-1hello world20!!!");
	cr_assert_eq(actual, expected);
}

Test(conversions, verify_10_results_in_ah, .init = setup, .fini = teardown,
	.exit_code =
	EXIT_SUCCESS)
{
	char *expected = "a";
	char *hex = NULL;
	convert_uint64_to_hex(10, &hex, false);

	cr_assert_str_eq(hex, expected);
	free(hex);
}

Test(conversions, verify_15_results_in_Fh, .init = setup, .fini = teardown,
	.exit_code = EXIT_SUCCESS)
{
	char *expected = "f";
	char *hex = NULL;
	convert_uint64_to_hex(15, &hex, false);

	cr_assert_str_eq(hex, expected);
	free(hex);
}

Test(conversions, verify_20_results_in_14h, .init = setup, .fini = teardown,
     .exit_code = EXIT_SUCCESS)
{
	char *expected = "14";
	char *hex = NULL;
	convert_uint64_to_hex(20, &hex, false);

	cr_assert_str_eq(hex, expected);
	free(hex);
}

Test(conversions, verify_hex_str_converts_to_uint64_t, .init = setup,
	.fini = teardown, .exit_code = EXIT_SUCCESS)
{
	uint64_t expected = 20;
	uint64_t actual = 0;
	bool parsed = str_hex_to_uint64("0x14", &actual);

	cr_assert(parsed);
	cr_assert_eq(actual, expected, "%" PRIu64 " == %" PRIu64, actual, expected);
}

Test(conversions, verify_lowercase_hex_str_converts_to_uint64_t, .init = setup,
     .fini = teardown, .exit_code = EXIT_SUCCESS)
{
	uint64_t expected = 240;
	uint64_t actual = 0;
	bool parsed = str_hex_to_uint64("0Xf0", &actual);

	cr_assert(parsed);
	cr_assert_eq(actual, expected, "%" PRIu64 " == %" PRIu64, actual, expected);
}