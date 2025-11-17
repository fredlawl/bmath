#include <asm-generic/errno-base.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unity/unity.h>

#include "../libbmath/src/lexer.h"
#include "../libbmath/src/token.h"

static struct lexer *lexer;

void setUp(void)
{
	struct lexer_settings lexer_settings;
	lexer = lexer_new(&lexer_settings);
}

void tearDown(void)
{
	lexer_free(lexer);
}

void test_basic_validation()
{
	struct tcase {
		char *name;
		char *text;
		int expected_err;
		struct token expected_token;
	};

	struct tcase cases[] = {
		{ "hex exceeds",
		  "0xaabbccddeeff00",
		  EINVAL,
		  { .type = TOK_NUMBER, .len = 16, .line = 0, .offset = 0 } },
		{ "hex invalid",
		  "0xgg",
		  EINVAL,
		  { .type = TOK_NUMBER, .len = 2, .line = 0, .offset = 0 } },
		{ "hex",
		  "0xaa",
		  EOF,
		  { .type = TOK_NUMBER, .len = 4, .line = 0, .offset = 0 } },
		{ "octal exceeds",
		  "07777777777777777777777",
		  EINVAL,
		  { .type = TOK_NUMBER, .len = 23, .line = 0, .offset = 0 } },
		{ "octal invalid",
		  "09",
		  EINVAL,
		  { .type = TOK_NUMBER, .len = 1, .line = 0, .offset = 0 } },
		{ "octal",
		  "0777",
		  EOF,
		  { .type = TOK_NUMBER, .len = 4, .line = 0, .offset = 0 } },
		{ "zero",
		  "0",
		  EOF,
		  { .type = TOK_NUMBER, .len = 1, .line = 0, .offset = 0 } },
		{ "one",
		  "1",
		  EOF,
		  { .type = TOK_NUMBER, .len = 1, .line = 0, .offset = 0 } },
		{ "unknown identifier",
		  ".",
		  EINVAL,
		  { .type = TOK_IDENT, .len = 1, .line = 0, .offset = 0 } },
		{ "identifier",
		  "yoo",
		  EOF,
		  { .type = TOK_IDENT, .len = 3, .line = 0, .offset = 0 } },
		{ "variable with number",
		  "@myvar1",
		  EOF,
		  { .type = TOK_VARIABLE, .len = 7, .line = 0, .offset = 0 } },
		{ "no identifier for variable",
		  "@",
		  EINVAL,
		  { .type = TOK_VARIABLE, .len = 1, .line = 0, .offset = 0 } },
		{ "no identifier for variable with trailing whitespace",
		  "@ ",
		  EINVAL,
		  { .type = TOK_VARIABLE, .len = 1, .line = 0, .offset = 0 } },
		{ "empty comment",
		  "//",
		  EOF,
		  { .type = TOK_COMMENT, .len = 2, .line = 0, .offset = 0 } },
		{ "empty comment 2",
		  "#",
		  EOF,
		  { .type = TOK_COMMENT, .len = 1, .line = 0, .offset = 0 } },
		{ "non empty comment",
		  "// something",
		  EOF,
		  { .type = TOK_COMMENT,
		    .len = sizeof("// something") - 1,
		    .line = 0,
		    .offset = 0 } },
		{ "non empty comment 2",
		  "# something",
		  EOF,
		  { .type = TOK_COMMENT,
		    .len = sizeof("# something") - 1,
		    .line = 0,
		    .offset = 0 } },
		{ "comment up to newline",
		  "// something\n",
		  EOF,
		  { .type = TOK_COMMENT,
		    .len = sizeof("// something") - 1,
		    .line = 0,
		    .offset = 0 } },
	};

	for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
		struct tcase tcase = cases[i];
		char assert_buf[256] = { 0 };
		struct token token;
		size_t bytes_written;

		bytes_written = snprintf(assert_buf, sizeof(assert_buf) - 1,
					 "test: %s: ", tcase.name);

		lexer_init(lexer, tcase.text, strlen(tcase.text));

		char token_a_str_buff[64] = { 0 };
		char token_b_str_buff[64] = { 0 };
		int error;

		token = lexer_next_token(lexer);
		error = lexer_errno(lexer);
		if (error) {
			sprintf(assert_buf + bytes_written,
				"expected correct errno");
			TEST_ASSERT_EQUAL_MESSAGE(tcase.expected_err, error,
						  assert_buf);
		}

		if (token_cmp(&token, &tcase.expected_token)) {
			token_str(&tcase.expected_token, token_a_str_buff,
				  sizeof(token_a_str_buff) - 1);
			token_str(&token, token_b_str_buff,
				  sizeof(token_b_str_buff) - 1);
			sprintf(assert_buf + bytes_written,
				"expected token %s does not match %s",
				token_a_str_buff, token_b_str_buff);
			TEST_FAIL_MESSAGE(assert_buf);
			break;
		}
	}
}

void test_combinations()
{
	struct tcase {
		char *name;
		char *text;
		int expected_err;
		struct token expected_tokens[32];
	};

	struct tcase cases[] = {
		{ "weird octal",
		  "079",
		  EOF,
		  { { .type = TOK_NUMBER, .len = 2, .line = 0, .offset = 0 },
		    { .type = TOK_NUMBER, .len = 1, .line = 0, .offset = 2 } } },
	};

	for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
		struct tcase tcase = cases[i];
		char assert_buf[256] = { 0 };
		struct token token;
		int token_idx = 0;
		size_t bytes_written;

		bytes_written = snprintf(assert_buf, sizeof(assert_buf) - 1,
					 "test: %s: ", tcase.name);

		lexer_init(lexer, tcase.text, strlen(tcase.text));

		while (true) {
			char token_a_str_buff[64] = { 0 };
			char token_b_str_buff[64] = { 0 };
			int error;

			token = lexer_next_token(lexer);
			if (token.type == TOK_NULL) {
				break;
			}

			error = lexer_errno(lexer);
			if (error) {
				sprintf(assert_buf + bytes_written,
					"expected correct errno");
				TEST_ASSERT_EQUAL_MESSAGE(tcase.expected_err,
							  error, assert_buf);
				break;
			}

			if (token_cmp(&token,
				      &tcase.expected_tokens[token_idx])) {
				token_str(&tcase.expected_tokens[token_idx],
					  token_a_str_buff,
					  sizeof(token_a_str_buff) - 1);
				token_str(&token, token_b_str_buff,
					  sizeof(token_b_str_buff) - 1);
				sprintf(assert_buf + bytes_written,
					"expected token %s does not match %s",
					token_a_str_buff, token_b_str_buff);
				TEST_FAIL_MESSAGE(assert_buf);
				break;
			}

			token_idx++;
		}
	}
}

void test_line_validation()
{
	struct tcase {
		char *name;
		char *text;
		int expected_err;
		struct token expected_tokens[32];
	};

	struct tcase cases[] = {
		{ "empty line", "", EOF, {} },
		{ "single token",
		  "1",
		  EOF,
		  { { .type = TOK_NUMBER, .len = 1, .line = 0, .offset = 0 } } },
		{ "multi lines",
		  "1\n2",
		  EOF,
		  { { .type = TOK_NUMBER, .len = 1, .line = 0, .offset = 0 },
		    { .type = TOK_NUMBER, .len = 1, .line = 1, .offset = 0 } } },
		{ "multi lines multiple tokens per line",
		  "1\n2 3",
		  EOF,
		  { { .type = TOK_NUMBER, .len = 1, .line = 0, .offset = 0 },
		    { .type = TOK_NUMBER, .len = 1, .line = 1, .offset = 0 },
		    { .type = TOK_NUMBER, .len = 1, .line = 1, .offset = 2 } } },
		{ "trailing lines",
		  "1234\n\n\n\n",
		  EOF,
		  {
			  { .type = TOK_NUMBER,
			    .len = 4,
			    .line = 0,
			    .offset = 0 },
		  } },
		{ "leading whitespace",
		  "\t\n1",
		  EOF,
		  { { .type = TOK_NUMBER, .len = 1, .line = 1, .offset = 0 } } },
	};

	for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
		struct tcase tcase = cases[i];
		char assert_buf[256] = { 0 };
		struct token token;
		int token_idx = 0;
		size_t bytes_written;

		bytes_written = snprintf(assert_buf, sizeof(assert_buf) - 1,
					 "test: %s: ", tcase.name);

		lexer_init(lexer, tcase.text, strlen(tcase.text));

		while (true) {
			char token_a_str_buff[64] = { 0 };
			char token_b_str_buff[64] = { 0 };
			int error;

			token = lexer_next_token(lexer);
			if (token.type == TOK_NULL) {
				break;
			}

			error = lexer_errno(lexer);
			if (error) {
				sprintf(assert_buf + bytes_written,
					"expected correct errno");
				TEST_ASSERT_EQUAL_MESSAGE(tcase.expected_err,
							  error, assert_buf);
				break;
			}

			if (token_cmp(&token,
				      &tcase.expected_tokens[token_idx])) {
				token_str(&tcase.expected_tokens[token_idx],
					  token_a_str_buff,
					  sizeof(token_a_str_buff) - 1);
				token_str(&token, token_b_str_buff,
					  sizeof(token_b_str_buff) - 1);
				sprintf(assert_buf + bytes_written,
					"expected token %s does not match %s",
					token_a_str_buff, token_b_str_buff);
				TEST_FAIL_MESSAGE(assert_buf);
				break;
			}

			token_idx++;
		}
	}
}

void test_infinite_loop()
{
	struct token token;
	char *text = "\n1\n";

	lexer_init(lexer, text, strlen(text));
	token = lexer_next_token(lexer);
	TEST_ASSERT_EQUAL_MESSAGE(TOK_NUMBER, token.type, "token is number");
	token = lexer_next_token(lexer);
	TEST_ASSERT_EQUAL_MESSAGE(TOK_NULL, token.type, "token is null");
}

int main(void)
{
	UNITY_BEGIN();
	RUN_TEST(test_basic_validation);
	RUN_TEST(test_combinations);
	RUN_TEST(test_line_validation);
	RUN_TEST(test_infinite_loop);
	return UNITY_END();
}
