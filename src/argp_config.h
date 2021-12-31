#ifndef BMATH_ARGP_CONFIG_H
#define BMATH_ARGP_CONFIG_H

#include <stdbool.h>
#include <strings.h>

#include "argp.h"

#ifdef VERSION
const char *argp_program_version = VERSION;
#else
const char *argp_program_version = "1.0.0";
#endif

const char *argp_program_bug_address =
	"Frederick Lawler <fred@fredlawl.com>";

static char args_doc[] = "";

static char doc[] =
	"Write out a term or expression to get a desired result."
	" Only POSITIVE numbers are allowed."
	"\n\nDetached Example:"
	"\n\t./bmath -d \"1 << 1\""
	"\n\t./bmath -d \"0x001\""
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
	char *detached_expr;
	bool should_uppercase_hex;
	bool should_show_unicode;
};

enum argument_opts {
    OPT_UPPERCASE = 'u',
    OPT_DETACHED = 'd',
    OPT_UNICODE = 128
};

static struct argp_option options[] = {
	{"uppercase", OPT_UPPERCASE, 0, OPTION_ARG_OPTIONAL, "Uppercase hex output", 0},
	{"detached", OPT_DETACHED, "EXPR", 0, "Execute single expression", 0},
    {"unicode", OPT_UNICODE, 0, OPTION_ARG_OPTIONAL, "Print unicode characters", 0},
	{0}
};

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
	struct arguments *arguments = (struct arguments *) state->input;

	switch (key) {
		case OPT_UPPERCASE: arguments->should_uppercase_hex = true; break;
		case OPT_DETACHED: arguments->detached_expr = arg; break;
        case OPT_UNICODE: arguments->should_show_unicode = true; break;
		case ARGP_KEY_ARG:
			if (state->arg_num > 0)
				argp_usage(state);

			break;
		case ARGP_KEY_END:
			if (state->arg_num > 0)
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
