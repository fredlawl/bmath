#include <locale.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// readline doesn't have FILE declared
#include <readline/history.h>
#include <readline/readline.h>

#include "argp.h"
#include "argp_config.h"
#include "parser.h"
#include "print.h"
#include "util.h"

#ifndef VERSION
#include "version.h"
const char *argp_program_version = VERSION;
#endif

static bool uppercase_hex = false;
static bool show_unicode = false;
static bool show_binary = false;

#define P_MAX_EXP_LEN 512

struct parse_expression {
	const char *expr;
	size_t len;
};

static int _eval(struct parser_context *ctx,
		 const struct parse_expression *expr, uint64_t *out)
{
	int err;

	err = parse(ctx, expr->expr, expr->len, out);
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
	}

	return err;
}

static int evaluate(struct parser_context *ctx, const char *expr, size_t len,
		    bool print_expr, uint64_t alignment)
{
	int err;
	uint64_t output = 0;

	err = _eval(ctx, &(struct parse_expression){ expr, len }, &output);
	if (err) {
		return err;
	}

	if (print_expr) {
		puts(expr);
	}

	print_set_stream(stdout);
	print_number(output, uppercase_hex,
		     (show_unicode) ? ENC_ALL : ENC_ASCII);

	if (alignment) {
		print_alignment(alignment, output, uppercase_hex);
	}

	if (show_binary) {
		print_binary(output);
	}

	return err;
}

static int do_readline(struct parser_context *ctx, uint64_t alignment)
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
		evaluate(ctx, input, strlen(input), false, alignment);
		fflush(stdout);

		free(input);
	}

	free(input);
	parser_free(ctx);
	return EXIT_SUCCESS;
}

static int do_stdin(struct parser_context *ctx, uint64_t alignment)
{
#define BUF_SIZE 4096
	int err;
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
				err = evaluate(ctx, expr, expr_index, true,
					       alignment);
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

	parser_free(ctx);

	return EXIT_SUCCESS;
}

int main(int argc, char *argv[])
{
	int err;
	char stdout_buff[4096] = { 0 };
	uint64_t alignment = 0;

	setvbuf(stdout, stdout_buff, _IOFBF, sizeof(stdout_buff));
	setlocale(LC_CTYPE, "en_US.UTF-8");

	struct arguments arguments;

	arguments.detached_expr = NULL;
	arguments.should_uppercase_hex = false;
	arguments.should_show_unicode = false;
	arguments.print_binary = false;

	argp_parse(&argp, argc, argv, 0, 0, &arguments);

	uppercase_hex = arguments.should_uppercase_hex;
	show_unicode = arguments.should_show_unicode;
	show_binary = arguments.print_binary;

	struct parser_settings settings = { .max_parse_len = P_MAX_EXP_LEN };

	struct parser_context *ctx = parser_new(&settings);

	if (arguments.alignment_expr) {
		err = _eval(ctx,
			    &(struct parse_expression){
				    arguments.alignment_expr,
				    strlen(arguments.alignment_expr) },
			    &alignment);
		if (err) {
			fprintf(stderr,
				"Unable to parse the align expression.");
			fflush(stdout);
			parser_free(ctx);
			return err;
		}
	}

	if (arguments.detached_expr) {
		err = evaluate(ctx, arguments.detached_expr,
			       strlen(arguments.detached_expr), false,
			       alignment);
		fflush(stdout);
		parser_free(ctx);
		return err;
	}

	if (isatty(0)) {
		return do_readline(ctx, alignment);
	}

	return do_stdin(ctx, alignment);
}
