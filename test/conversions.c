#include <criterion/criterion.h>
#include <inttypes.h>

#include "../src/conversions.h"

static void setup(void)
{
}

static void teardown(void)
{
}

Test(conversions, verify_hex_str_converts_to_uint64_t, .init = setup,
     .fini = teardown, .exit_code = EXIT_SUCCESS)
{
	uint64_t expected = 20;
	uint64_t actual = 0;
	bool parsed = str_hex_to_uint64("0x14", 4, &actual);

	cr_assert(parsed);
	cr_assert_eq(actual, expected, "%" PRIu64 " == %" PRIu64, actual,
		     expected);
}

Test(conversions, verify_lowercase_hex_str_converts_to_uint64_t, .init = setup,
     .fini = teardown, .exit_code = EXIT_SUCCESS)
{
	uint64_t expected = 240;
	uint64_t actual = 0;
	bool parsed = str_hex_to_uint64("0Xf0", 4, &actual);

	cr_assert(parsed);
	cr_assert_eq(actual, expected, "%" PRIu64 " == %" PRIu64, actual,
		     expected);
}
