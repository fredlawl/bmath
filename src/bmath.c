#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

#include "argp_config.h"
#include "conversions.h"
#include "parser.h"

#define __repeat_character(fp, n, c)                    \
do {                                                    \
	for (int i = 0; i < n; i++) fputc(c, fp);       \
} while (0)

static void print_number(uint64_t num, bool uppercase_hex)
{
	char *hex = NULL;
	size_t hex_len = 0;

	printf("  Dec: %" PRIu64 "\n", num);

	if (num <= CHAR_MAX) {
		if (num <= 31) {
			printf(" Char: <special>\n");
		} else {
			printf(" Char: %c\n", (char) num);
		}
	} else {
		printf(" Char: Exceeded\n");
	}

	if ((hex_len = convert_uint64_to_hex(num, &hex, uppercase_hex)) > 0) {
		printf("  Hex: 0x%s\n", hex);

		if (num <= UINT16_MAX) {
			printf("Hex16: 0x");
			__repeat_character(stdout, (4 - (int) hex_len), '0');
			printf("%s\n", hex);
		} else {
			printf("Hex16: Exceeded\n");
		}

		if (num <= UINT32_MAX) {
			printf("Hex32: 0x");
			__repeat_character(stdout, (8 - (int) hex_len), '0');
			printf("%s\n", hex);
		} else {
			printf("Hex32: Exceeded\n");
		}

		printf("Hex64: 0x");
		__repeat_character(stdout, (16 - (int) hex_len), '0');
		printf("%s\n", hex);

		free(hex);
	}
}

int main(int argc, char *argv[])
{
	struct arguments arguments;
	char *input;

	arguments.should_uppercase_hex = false;

	argp_parse(&argp, argc, argv, 0, 0, &arguments);

	input = arguments.args[0];

	uint64_t output = 0;
	int result = parse(input, &output);
	if (result <= 0) {
		// notify error...
	}

	print_number(output, arguments.should_uppercase_hex);

	return EXIT_SUCCESS;
}
