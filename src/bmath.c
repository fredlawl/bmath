#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <locale.h>
#include <iconv.h>

#include "argp_config.h"
#include "parser.h"
#include "util.h"

static bool uppercaseHex = false;
static bool showUnicode = false;

enum encoding_t {
    UTF8,
    UTF16,
    UTF32
};

static const char* to_encoding_lookup[] = {
    [UTF8] = "UTF-8",
    [UTF16] = "UTF-16BE",
    [UTF32] = "UTF-32BE"
};

static const char* from_encoding_lookup[] = {
    [UTF8] = "UTF-8",
    [UTF16] = "UTF-16LE",
    [UTF32] = "UTF-32LE"
};

static const char* to_encoding_pretty_print_lookup[] = {
    [UTF8] = " UTF-8",
    [UTF16] = "UTF-16",
    [UTF32] = "UTF-32"
};


static void print_unicode(uint64_t num, bool uppercase_hex, enum encoding_t from_unicode, enum encoding_t to_unicode) {
    iconv_t cd;

    char number_as_byte_array[8] = {0};
    char* utf8_input = number_as_byte_array;
    char* to_unicode_input = number_as_byte_array;
    size_t in_size = sizeof number_as_byte_array;
    size_t in_bytes_size = sizeof number_as_byte_array;

    char utf8_buf[8] = {0};
    char to_unicode_buf[8] = {0};
    char* utf8 = utf8_buf;
    char* to_unicode_bytes = to_unicode_buf;
    size_t utf8_size = sizeof utf8_buf;
    size_t to_unicode_size = sizeof to_unicode_buf;

    memcpy(&number_as_byte_array, &num, sizeof num);

    printf("%s: ", to_encoding_pretty_print_lookup[to_unicode]);

    // Convert to UTF-8
    cd = iconv_open("UTF-8", from_encoding_lookup[from_unicode]);
    size_t utf8_conversion = iconv(cd, &utf8_input, &in_size, &utf8, &utf8_size);
    iconv_close(cd);

    if (utf8_conversion == (size_t) -1) {
        printf("<invalid> ");
    } else {
        printf("%s ", utf8_buf);
    }

    // Convert from_unicode to to_unicode
    cd = iconv_open(to_encoding_lookup[to_unicode], from_encoding_lookup[from_unicode]);
    size_t conversion = iconv(cd, &to_unicode_input, &in_bytes_size, &to_unicode_bytes, &to_unicode_size);
    iconv_close(cd);

    if (conversion == (size_t) -1) {
        printf("(no bytes)\n");
        return;
    }

    printf("(0x");

    /*
        Not great, but this is easier than counting bits to determine how many next bytes
        belong to the same character.

        This kind of works because iconv decrements the output buffer size, and we know
        based on UTF8-16 if there's 1-2 null bytes at the end of our buffer, and with UTF32,
        the null character is 4 bytes.
     */
    size_t offset = 1;
    if (to_unicode == UTF16) {
        offset = 2;
    }

    if (to_unicode == UTF32) {
        offset = 4;
    }

    for (size_t i = 0; i < 8 - (to_unicode_size + offset); i++) {
        __print_hex((uint64_t) 0xff & to_unicode_buf[i], 2, uppercase_hex);
    }

    printf(")\n");
}

static void print_number(uint64_t num, bool uppercase_hex)
{
	printf("   Dec: %" PRIu64 "\n", num);

    if (!showUnicode) {
        if (num <= CHAR_MAX) {
            if (num <= 31) {
                printf("  Char: <special>\n");
            } else {
                printf("  Char: %c\n", (char) num);
            }
        } else {
            printf("  Char: Exceeded\n");
        }
    } else {
        if (num <= UINT32_MAX) {
            print_unicode(num, uppercase_hex, UTF32, UTF8);
            print_unicode(num, uppercase_hex, UTF32, UTF16);
            print_unicode(num, uppercase_hex, UTF32, UTF32);
        } else {
            printf("%s: Exceeded\n", to_encoding_pretty_print_lookup[UTF8]);
            printf("%s: Exceeded\n", to_encoding_pretty_print_lookup[UTF16]);
            printf("%s: Exceeded\n", to_encoding_pretty_print_lookup[UTF32]);
        }
    }

	printf("   Hex: 0x");
	__print_hex(num, 0, uppercase_hex);
	printf("\n");

	if (num <= UINT16_MAX) {
		printf(" Hex16: 0x");
		__print_hex(num, 4, uppercase_hex);
		printf("\n");
	} else {
		printf(" Hex16: Exceeded\n");
	}

	if (num <= UINT32_MAX) {
		printf(" Hex32: 0x");
		__print_hex(num, 8, uppercase_hex);
		printf("\n");
	} else {
		printf(" Hex32: Exceeded\n");
	}

	printf(" Hex64: 0x");
	__print_hex(num, 16, uppercase_hex);
	printf("\n");
}

int evaluate(const char* input)
{
	uint64_t output = 0;
	int result = parse(input, &output);
	if (result <= 0) {
		// notify error...
		return EXIT_FAILURE;
	}

	print_number(output, uppercaseHex);
	return EXIT_SUCCESS;
}

int main(int argc, char *argv[])
{
    setlocale(LC_CTYPE,"en_US.UTF-8");

	struct arguments arguments;
	char *input;

	arguments.detached_expr = NULL;
	arguments.should_uppercase_hex = false;
    arguments.should_show_unicode = false;

	argp_parse(&argp, argc, argv, 0, 0, &arguments);

    uppercaseHex = arguments.should_uppercase_hex;
    showUnicode = arguments.should_show_unicode;

	if (arguments.detached_expr) {
		return evaluate(arguments.detached_expr);
	}

	while (true) {
		input = readline("expr> ");

		if (!input)
			break;

		if (strcasecmp(input, "exit") == 0 ||
		    strcasecmp(input,"quit") == 0) {
			break;
		}

		add_history(input);
		evaluate(input);

		free(input);
	}

	free(input);

	return EXIT_SUCCESS;
}
