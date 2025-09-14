#include <asm-generic/errno-base.h>
#include <stddef.h>
#include <stdio.h>
#include <stddef.h>
#include <sys/types.h>
#include <unity/unity.h>

#include "../src/print.h"
#include "../src/config.h"

void setUp(void)
{
}

void tearDown(void)
{
}

void test_parse_encodings_list()
{
	struct test_case {
		const char *name;
		const char *str;
		ssize_t array_len;
		enum encoding_t expected
			[100]; // suppress annoying array inialization errors
	} cases[] = {
		{ "unknown passed", "test", -EINVAL, {} },
		{ "empty is OK", "", 0, {} },
		{ "parses 1", "uint", 1, { ENC_UINT } },
		{ "parses 2", "uint,int", 2, { ENC_UINT, ENC_INT } },
		{ "parses 2 with spaces",
		  "  uint  , int",
		  2,
		  { ENC_UINT, ENC_INT } },
		{ "parses 2 with trailing comma",
		  "uint,int,",
		  2,
		  { ENC_UINT, ENC_INT } },
		{ "parses with prefix and appended comma",
		  ",uint,int,",
		  2,
		  { ENC_UINT, ENC_INT } },
		{ "no duplicates",
		  "uint,int,uint",
		  -EINVAL,
		  { ENC_UINT, ENC_INT } }, // no duplicates
		{ "mixed case works", "Int", 1, { ENC_INT } },
		{ "comma", ",", 0, {} },
		{ "two comma", ",,", 0, {} },
		{
			"all",
			"all",
			ENC_LENGTH,
			{
				ENC_ASCII,
				ENC_BINARY,
				ENC_HEX,
				ENC_HEX16,
				ENC_HEX32,
				ENC_HEX64,
				ENC_INT,
				ENC_UINT,
				ENC_OCTAL,
				ENC_UNICODE,
				ENC_UTF8,
				ENC_UTF16,
				ENC_UTF32,
			},
		},
		{ "all only useable in isolation",
		  "int,all",
		  -EINVAL,
		  {
			  ENC_INT,
		  } },
	};

	for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
		struct test_case test_case = cases[i];
		enum encoding_t *encodings;
		ssize_t len;
		char test_msg[100] = { 0 };

		len = parse_encodings_list(test_case.str, &encodings);
		//if (len < 0) {
		//	TEST_FAIL_MESSAGE("ENOMEM");
		//}

		snprintf(test_msg, sizeof(test_msg),
			 "%s: array output mismatch", test_case.name);
		TEST_ASSERT_EQUAL_MESSAGE(test_case.array_len, len, test_msg);

		for (int j = 0; j < len; j++) {
			enum encoding_t encoding = encodings[j];
			snprintf(test_msg, sizeof(test_msg),
				 "%s: encoding matches", test_case.name);
			TEST_ASSERT_EQUAL_MESSAGE(test_case.expected[j],
						  encoding, test_msg);
		}

		free(encodings);
	}
}

void test_next_file()
{
	char *next_file = NULL;
	while ((next_file = locate_next_config_file("tesst.conf"))) {
		fprintf(stderr, "next file: %s\n", next_file);
		free(next_file);
	}
}

int main(void)
{
	UNITY_BEGIN();
	RUN_TEST(test_parse_encodings_list);
	RUN_TEST(test_next_file);
	return UNITY_END();
}
