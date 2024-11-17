#include "argp.h"
#include <iconv.h>
#include <inttypes.h>
#include <locale.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "argp_config.h"
#include "parser.h"
#include "util.h"

static bool uppercaseHex = false;
static bool showUnicode = false;
static bool printBinary = false;

enum encoding_t { UTF8, UTF16, UTF32 };

static const char *to_encoding_lookup
	[] = { [UTF8] = "UTF-8", [UTF16] = "UTF-16BE", [UTF32] = "UTF-32BE" };

static const char *from_encoding_lookup
	[] = { [UTF8] = "UTF-8", [UTF16] = "UTF-16LE", [UTF32] = "UTF-32LE" };

static const char *to_encoding_pretty_print_lookup
	[] = { [UTF8] = " UTF-8", [UTF16] = "UTF-16", [UTF32] = "UTF-32" };

static iconv_t iconv_descriptors[3] = { 0 };

static void print_unicode(uint64_t num, bool uppercase_hex,
			  enum encoding_t to_unicode)
{
	iconv_t cd;
	size_t conversion;

	char number_as_byte_array[8] = { 0 };
	char *utf8_input = number_as_byte_array;
	char *to_unicode_input = number_as_byte_array;
	size_t in_size = sizeof number_as_byte_array;
	size_t in_bytes_size = sizeof number_as_byte_array;

	char utf8_buf[8] = { 0 };
	char *utf8 = utf8_buf;
	char to_unicode_buf[8] = { 0 };
	char *to_unicode_bytes = to_unicode_buf;
	size_t utf8_size = sizeof utf8_buf;
	size_t to_unicode_size = sizeof to_unicode_buf;

	size_t offset[3] = { 1, 2, 4 };

	memcpy(&number_as_byte_array, &num, sizeof num);

	printf("%s: ", to_encoding_pretty_print_lookup[to_unicode]);

	// Convert to UTF-8
	if (num < 31) {
		fputs("<special> ", stdout);
	} else {
		cd = iconv_descriptors[UTF8];
		conversion =
			iconv(cd, &utf8_input, &in_size, &utf8, &utf8_size);

		if (conversion == (size_t)-1) {
			fputs("<invalid> ", stdout);
		} else {
			printf("%s ", utf8_buf);
		}
	}

	// Convert from_unicode to to_unicode
	cd = iconv_descriptors[to_unicode];
	conversion = iconv(cd, &to_unicode_input, &in_bytes_size,
			   &to_unicode_bytes, &to_unicode_size);

	if (conversion == (size_t)-1) {
		puts("<invalid>");
		return;
	}

	fputs("(0x", stdout);

	/*
	 re: offset
	 This kind of works because iconv decrements the output buffer size, and we
	 know based on UTF8-16 if there's 1-2 null bytes at the end of our buffer,
	 and with UTF32, the null character is 4 bytes.
	 */
	for (size_t i = 0; i < 8 - (to_unicode_size + offset[to_unicode]);
	     i++) {
		__print_hex((uint64_t)0xff & to_unicode_buf[i], 2,
			    uppercase_hex);
	}

	puts(")");
}

static void print_binary(uint64_t number)
{
	uint64_t mask;
	uint64_t counter = 64;
	// (bytes * bits per byte) + 2 newlines + 8 spaces + 1 null
	char buff[(sizeof(number) * 8) + 2 + 8 + 1] = { 0 };
	int i = 0;
	while (counter > 0) {
		mask = (uint64_t)1 << (counter - 1);
		if ((counter % 8) == 0) {
			buff[i] = ' ';
			i++;
		}
		// ascii 0 = 48
		buff[i] = ((number & mask) == mask) + 48;
		i++;
		counter--;
		if ((counter % 32) == 0) {
			buff[i] = '\n';
			i++;
		}
	}

	fwrite(buff, sizeof(buff), 1, stdout);
}

static void print_number(uint64_t num, bool uppercase_hex)
{
	printf("   u64: %" PRIu64 "\n", num);

	if (num <= 0xff) {
		printf("    i8: %" PRId8 "\n", (int8_t)num);
	} else if (num <= 0xffff) {
		printf("   i16: %" PRId16 "\n", (int16_t)num);
	} else if (num <= 0xffffffff) {
		printf("   i32: %" PRId32 "\n", (int32_t)num);
	} else {
		printf("   i64: %" PRId64 "\n", (int64_t)num);
	}

	if (!showUnicode) {
		if (num <= CHAR_MAX) {
			if (num <= 31) {
				puts("  char: <special>");
			} else {
				printf("  char: %c\n", (char)num);
			}
		} else {
			puts("  char: Exceeded");
		}
	} else {
		if (num <= UINT32_MAX) {
			print_unicode(num, uppercase_hex, UTF8);
			print_unicode(num, uppercase_hex, UTF16);
			print_unicode(num, uppercase_hex, UTF32);
		} else {
			printf("%s: Exceeded\n",
			       to_encoding_pretty_print_lookup[UTF8]);
			printf("%s: Exceeded\n",
			       to_encoding_pretty_print_lookup[UTF16]);
			printf("%s: Exceeded\n",
			       to_encoding_pretty_print_lookup[UTF32]);
		}
	}

	fputs("   Hex: 0x", stdout);
	__print_hex(num, 0, uppercase_hex);
	putchar('\n');

	if (num <= UINT16_MAX) {
		fputs(" Hex16: 0x", stdout);
		__print_hex(num, 4, uppercase_hex);
		putchar('\n');
	} else {
		puts(" Hex16: Exceeded");
	}

	if (num <= UINT32_MAX) {
		fputs(" Hex32: 0x", stdout);
		__print_hex(num, 8, uppercase_hex);
		putchar('\n');
	} else {
		puts(" Hex32: Exceeded");
	}

	fputs(" Hex64: 0x", stdout);
	__print_hex(num, 16, uppercase_hex);
	putchar('\n');

	if (printBinary) {
		print_binary(num);
	}
}

int evaluate(const char *input, size_t len, bool print_expr)
{
	uint64_t output = 0;
	int err = parse(input, len, &output);
	if (err) {
		switch (err) {
		case PE_NOTHING_TO_PARSE:
			fputs("Nothing to parse.\n", stderr);
			break;
		case PE_EXPRESSION_TOO_LONG:
			fputs("Expression too long.\n", stderr);
			break;
		case PE_PARSE_ERROR:
			fputs("Parse error ocurred.\n", stderr);
			break;
		default:
			fputs("Unknown error ocurred.\n", stderr);
		}
		return err;
	}

	if (print_expr) {
		puts(input);
	}
	print_number(output, uppercaseHex);
	return err;
}

static int do_readline()
{
	char *input;

	while (true) {
		input = readline("expr> ");

		if (!input)
			break;

		if (strcasecmp(input, "exit") == 0 ||
		    strcasecmp(input, "quit") == 0) {
			break;
		}

		add_history(input);
		evaluate(input, strlen(input), false);
		fflush(stdout);

		free(input);
	}

	free(input);
	return EXIT_SUCCESS;
}

static int do_stdin()
{
#define BUF_SIZE 4096
	ssize_t bytes_read = 0;
	ssize_t expr_index = 0;
	char expr[P_MAX_EXP_LEN] = { 0 };

	do {
		char read_buff[BUF_SIZE] = { 0 };
		ssize_t buff_index = 0;

		bytes_read = read(STDIN_FILENO, read_buff, sizeof(read_buff));
		if (bytes_read < 0) {
			perror("do_stdin() error reading line");
			return EXIT_FAILURE;
		}

		if (bytes_read == 0)
			break;

		while (buff_index < bytes_read) {
			if (unlikely(expr_index > P_MAX_EXP_LEN)) {
				fputs("Attempted input buffer overflow. Skipping.\n",
				      stderr);
				memset(expr, '\0', P_MAX_EXP_LEN);
				expr_index = 0;
				while (buff_index++ < bytes_read &&
				       read_buff[buff_index] != '\n')
					;
				buff_index++;
				continue;
			}

			if (read_buff[buff_index] == '\n') {
				int err = evaluate(expr, expr_index, true);
				if (!err) {
					puts("");
				}
				memset(expr, '\0', expr_index);
				expr_index = 0;
				buff_index++;
				continue;
			}

			expr[expr_index] = read_buff[buff_index];
			expr_index++;
			buff_index++;
		}

	} while (bytes_read > 0);

	fflush(stdout);

	return EXIT_SUCCESS;
}

int main(int argc, char *argv[])
{
	char stdout_buff[4096] = { 0 };
	setvbuf(stdout, stdout_buff, _IOFBF, sizeof(stdout_buff));
	setlocale(LC_CTYPE, "en_US.UTF-8");

	struct arguments arguments;

	arguments.detached_expr = NULL;
	arguments.should_uppercase_hex = false;
	arguments.should_show_unicode = false;
	arguments.print_binary = false;

	argp_parse(&argp, argc, argv, 0, 0, &arguments);

	uppercaseHex = arguments.should_uppercase_hex;
	showUnicode = arguments.should_show_unicode;
	printBinary = arguments.print_binary;

	if (showUnicode) {
		iconv_descriptors[0] = iconv_open(to_encoding_lookup[UTF8],
						  from_encoding_lookup[UTF32]);
		iconv_descriptors[1] = iconv_open(to_encoding_lookup[UTF16],
						  from_encoding_lookup[UTF32]);
		iconv_descriptors[2] = iconv_open(to_encoding_lookup[UTF32],
						  from_encoding_lookup[UTF32]);
	}

	if (arguments.detached_expr) {
		int err = evaluate(arguments.detached_expr,
				   strlen(arguments.detached_expr), false);
		fflush(stdout);
		return err;
	}

	if (isatty(0)) {
		return do_readline();
	}

	return do_stdin();
}
