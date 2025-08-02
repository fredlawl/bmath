#pragma once

#include <stdbool.h>
#include <strings.h>

#include "argp.h"

const char *argp_program_bug_address = "Frederick Lawler <fred@fredlawl.com>";

static char args_doc[] = "";

static char doc[] = "\nUsage examples:"
		    "\n\t./bmath -d \"0x001\""
		    "\n\t./bmath < input-file"
		    "\n\t./bmath"
		    "\n\nSee bmath(1) for detailed examples and explinations.";

struct arguments {
	char *detached_expr;
	char *alignment_expr;
	bool should_uppercase_hex;
	bool should_show_unicode;
	bool print_binary;
};

enum argument_opts {
	OPT_UPPERCASE = 'u',
	OPT_BINARY = 'b',
	OPT_UNICODE = 128,
	OPT_DETACHED = 'd',
	OPT_ALIGN = 'a'
};

static struct argp_option options[] = {
	{ "uppercase", OPT_UPPERCASE, 0, OPTION_ARG_OPTIONAL,
	  "Uppercase hex output", 0 },
	{ "binary", OPT_BINARY, 0, 0, "Print the result in binary", 0 },
	{ "unicode", OPT_UNICODE, 0, OPTION_ARG_OPTIONAL,
	  "Print unicode characters", 0 },
	{ "align", OPT_ALIGN, "EXPR", 0,
	  "Print input expression's alignment according to alignment expression. Alignment expression should be power of 2, but it's not enforced",
	  0 },
	{ "detached", OPT_DETACHED, "EXPR", 0, "Execute single expression", 0 },
	{ 0 }
};

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
	struct arguments *arguments = (struct arguments *)state->input;

	switch (key) {
	case OPT_UPPERCASE:
		arguments->should_uppercase_hex = true;
		break;
	case OPT_BINARY:
		arguments->print_binary = true;
		break;
	case OPT_UNICODE:
		arguments->should_show_unicode = true;
		break;
	case OPT_DETACHED:
		arguments->detached_expr = arg;
		break;
	case OPT_ALIGN:
		arguments->alignment_expr = arg;
		break;
	case ARGP_KEY_ARG:
		if (state->arg_num > 0)
			argp_usage(state);

		break;
	case ARGP_KEY_END:
		if (state->arg_num > 0)
			argp_usage(state);

		break;
	default:
		return ARGP_ERR_UNKNOWN;
	}

	return 0;
}

static struct argp argp = (struct argp){ .options = options,
					 .parser = parse_opt,
					 .args_doc = args_doc,
					 .doc = doc,
					 .children = NULL,
					 .help_filter = NULL,
					 .argp_domain = NULL };
