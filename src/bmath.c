#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "argp_config.h"
#include "conversions.h"
#include "parser.h"
#include "util.h"

static bool upercaseHex = false;

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

int evaluate(const char* input)
{
	uint64_t output = 0;
	int result = parse(input, &output);
	if (result <= 0) {
		// notify error...
		return EXIT_FAILURE;
	}

	print_number(output, upercaseHex);
	return EXIT_SUCCESS;
}

int main(int argc, char *argv[])
{
	struct arguments arguments;
	char *input;

	arguments.detached_expr = NULL;
	arguments.should_uppercase_hex = false;

	argp_parse(&argp, argc, argv, 0, 0, &arguments);

	upercaseHex = arguments.should_uppercase_hex;

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
