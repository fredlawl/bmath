#pragma once

#include "print.h"
#include <asm-generic/errno-base.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <strings.h>
#include <argp.h>

#include "config.h"

const char *argp_program_bug_address = "Frederick Lawler <me@fred.software>";

static char args_doc[] = "[EXPR]\n-w FILE";

static char doc[] = "\nUsage examples:"
		    "\n\t./bmath \"0x001\""
		    "\n\t./bmath < input-file"
		    "\n\t./bmath -w input-file"
		    "\n\t./bmath"
		    "\n\nSee bmath(1) for detailed examples and explinations.";

#define CFG_NONE 0UL
#define CFG_FMT_ENCODINGS (1UL << 0)
#define CFG_FMT_HUMAN (1UL << 1)
#define CFG_FMT_JUTSIFY (1UL << 2)
#define CFG_FMT_UPPERCASE (1UL << 3)

struct arguments {
	char *config_file;
	char *headless;
	char *watch_path;
	bool watch;
	bool no_newline;
	struct config *cfg;
	uint64_t cfg_changed;
};

enum argument_opts {
	OPT_CONFIG = 'c',
	OPT_ENCODINGS = 'e',
	OPT_NO_NEWLINE = 'n',
	OPT_FMT_HUMAN = 128,
	OPT_FMT_JUSTIFY = 129,
	OPT_FMT_UPPERCASE = 130,
	OPT_WATCH = 'w',
};

static struct argp_option options[] = {
	{ "config", OPT_CONFIG, "FILE", 0,
	  "Specify a confgiuration file for the program", 0 },
	{ "fmt-encodings", OPT_ENCODINGS, "ENCODINGS", 0,
	  "Comma separated list of encodings. Defaults to 'uint'. For all, use 'all'. Set to empty to not display anything. See bmath-config(5) for details",
	  0 },
	{ "fmt-human", OPT_FMT_HUMAN, 0, 0,
	  "Prefixes output with the type of data. See bmath-config(5) for details",
	  0 },
	{ "fmt-justify", OPT_FMT_JUSTIFY, 0, 0,
	  "Align the left side of output to the longest prefix up to ':', if --fmt-human is set. See bmath-config(5) for details",
	  0 },
	{ "fmt-uppercase", OPT_FMT_UPPERCASE, 0, 0,
	  "Uppercase hex output. See bmath-config(5) for details", 0 },
	{ "no-newline", OPT_NO_NEWLINE, 0, OPTION_NO_USAGE,
	  "Removes trailing newline on every printed result, except when an error occurs",
	  0 },
	{ "watch", OPT_WATCH, 0, OPTION_NO_USAGE,
	  "Watches file for changes. ie. Live reloading. When enabled, stdin capabilities are disabled, and requires a file path to input file as first program argument",
	  0 },
	{ 0 }
};

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
	ssize_t encodings;
	struct arguments *arguments = (struct arguments *)state->input;

	switch (key) {
	case OPT_CONFIG:
		arguments->config_file = arg;
		break;
	case OPT_ENCODINGS:
		// free previously allocated
		free(arguments->cfg->encoding_order);
		encodings = parse_encodings_list(
			arg, &arguments->cfg->encoding_order);
		if (encodings < 0) {
			argp_error(state, "Invalid value for -e: %s", arg);
			return EINVAL;
		}
		arguments->cfg->encoding_order_len = encodings;
		arguments->cfg_changed |= CFG_FMT_ENCODINGS;
		break;
	case OPT_FMT_HUMAN:
		arguments->cfg->enc_fmt |= FMT_HUMAN;
		arguments->cfg_changed |= CFG_FMT_HUMAN;
		break;
	case OPT_FMT_JUSTIFY:
		arguments->cfg->output_fmt |= OUT_FMT_JUSTIFY;
		arguments->cfg_changed |= CFG_FMT_JUTSIFY;
		break;
	case OPT_FMT_UPPERCASE:
		arguments->cfg->enc_fmt |= FMT_UPPERCASE;
		arguments->cfg_changed |= CFG_FMT_UPPERCASE;
		break;
	case OPT_NO_NEWLINE:
		arguments->no_newline = true;
		break;
	case OPT_WATCH:
		arguments->watch = true;
		break;
	case ARGP_KEY_ARG:
		if (arguments->watch && state->arg_num == 0) {
			arguments->watch_path = arg;
			break;
		}

		if (state->arg_num == 0) {
			arguments->headless = arg;
			break;
		}

		argp_error(state, "Invalid value for positional argument: %s",
			   arg);
		return EINVAL;
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
