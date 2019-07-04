#ifndef BMATH_ARGP_CONFIG_H
#define BMATH_ARGP_CONFIG_H

#include <stdbool.h>

#include "argp.h"

const char *argp_program_version = "0.0.1";
const char *argp_program_bug_address =
	"Frederick Lawler <fred@fredlawlcom>";

static char args_doc[] =
	"\"[EXPR]\"";

static char doc[] =
	"Write out a term or expression to get a desired result."
	" Only POSITIVE numbers are allowed."
	"\n\nExample:"
	"\n\t./bmath \"1 << 1\""
	"\n\t./bmath \"0x001\""
        "\n\nLanguage Specification:"
	"\n\texpr  = expr, op, term"
	"\n\t      | term ;"
	"\n\tterm = term, shift_op, factor"
	"\n\t     | factor ;"
	"\n\tfactor = number"
	"\n\t       | lparen, expr, rparen"
	"\n\t       | negate, factor ;"
	"\n\tnumber = digit, { digit }"
	"\n\t       | hex ;"
	"\n\tdigit = [0-9], { [0-9] } ;"
	"\n\thex = \"0x\", [0-9a-fA-F], { [0-9a-fA-F] } ;"
	"\n\tshift_op = \"<<\" | \">>\" ;"
	"\n\top = \"^\" | \"|\" | \"&\" ;"
	"\n\tlparen = \"(\" ;"
	"\n\trparen = \")\" ;"
	"\n\tnegate = \"~\" ;";

struct arguments
{
	char *args[1];
	bool should_uppercase_hex;
};

static struct argp_option options[] = {
	{"uppercase", 'u', 0, 0, "Uppercase hex output.", 0},
	{0}
};

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
	struct arguments *arguments = (struct arguments *) state->input;

	switch (key) {
		case 'u': arguments->should_uppercase_hex = true; break;
		case ARGP_KEY_ARG:
			if (state->arg_num > 1)
				argp_usage(state);

			arguments->args[state->arg_num] = arg;

			break;
		case ARGP_KEY_END:
			if (state->arg_num < 1)
				argp_usage(state);
			break;
		default: return ARGP_ERR_UNKNOWN;
	}

	return 0;
}

static struct argp argp = (struct argp) {
	.options = options,
	.parser = parse_opt,
	.args_doc = args_doc,
	.doc = doc,
	.children = NULL,
	.help_filter = NULL,
	.argp_domain = NULL
};

#endif
