#include <errno.h>
#include <string.h>
#include <unity/unity.h>

#include "../src/conversions.h"

void setUp()
{
}

void tearDown()
{
}

void test_verify_hex_str_converts_to_uint64_t()
{
	uint64_t expected = 20;
	uint64_t actual = 0;
	size_t len = strlen("0x14");
	ssize_t parsed = str_hex_to_uint64("0x14", len, &actual);

	TEST_ASSERT_EQUAL_MESSAGE(len, parsed, "bytes returned");
	TEST_ASSERT_EQUAL_MESSAGE(expected, actual, "parsed value");
}

void test_verify_uppercase_hex_str_converts_to_uint64_t()
{
	uint64_t expected = 240;
	uint64_t actual = 0;
	size_t len = strlen("0XF0");
	size_t parsed = str_hex_to_uint64("0XF0", len, &actual);

	TEST_ASSERT_EQUAL_MESSAGE(len, parsed, "bytes returned");
	TEST_ASSERT_EQUAL_MESSAGE(expected, actual, "parsed value");
}

void test_fail_invalid_hex()
{
	uint64_t actual = 0;
	size_t parsed = str_hex_to_uint64("0xhuh", 5, &actual);
	int err = errno;

	TEST_ASSERT_EQUAL_MESSAGE(EINVAL, err, "error matches");
	TEST_ASSERT_EQUAL_MESSAGE(0, parsed, "bytes returned");
}

void test_hex_exceeds_len()
{
	uint64_t actual = 0;
	size_t parsed = str_hex_to_uint64("0xaa", 3, &actual);
	int err = errno;

	TEST_ASSERT_EQUAL_MESSAGE(E2BIG, err, "error matches");
	TEST_ASSERT_EQUAL_MESSAGE(-4, parsed, "bytes returned");
}

void test_hex_allow_spaces()
{
	uint64_t actual = 0;
	size_t parsed = str_hex_to_uint64("0xa 0", 5, &actual);

	TEST_ASSERT_EQUAL_MESSAGE(3, parsed, "bytes returned");
	TEST_ASSERT_EQUAL_MESSAGE(10, actual, "parsed value");
}

int main(void)
{
	UNITY_BEGIN();
	RUN_TEST(test_verify_hex_str_converts_to_uint64_t);
	RUN_TEST(test_verify_uppercase_hex_str_converts_to_uint64_t);
	//	RUN_TEST(test_fail_invalid_hex);
	RUN_TEST(test_hex_exceeds_len);
	RUN_TEST(test_hex_allow_spaces);
	return UNITY_END();
}
