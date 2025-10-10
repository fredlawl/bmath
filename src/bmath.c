#include <argp.h>
#include <asm-generic/errno-base.h>
#include <fcntl.h>
#include <locale.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "config.h"
#include "argp_config.h"
#include "parser.h"
#include "execute.h"
#include "print.h"

#ifndef VERSION
#include "version.h"
const char *argp_program_version = VERSION;
#endif

static char *watch_file = NULL;

int main(int argc, char *argv[])
{
	int err;
	struct parser_settings settings;
	struct arguments arguments;
	FILE *err_stream;
	FILE *out_stream;
	struct execution_ctx ectx = { 0 };
	char stdout_buff[4096] = { 0 };
	char *cfg_path = NULL;

	err_stream = stderr;
	out_stream = stdout;

	setvbuf(out_stream, stdout_buff, _IOFBF, sizeof(stdout_buff));
	setlocale(LC_CTYPE, "en_US.UTF-8");

	arguments.config_file = NULL;
	arguments.headless = NULL;
	arguments.watch = false;
	arguments.no_newline = false;
	arguments.watch_path = NULL;
	arguments.cfg_changed = CFG_NONE;

	// Create baseline cfg in case args overwrite
	arguments.cfg = config_new();
	if (!arguments.cfg) {
		fprintf(err_stream, "Failed to create config\n");
		err = ENOMEM;
		goto err;
	}

	argp_parse(&argp, argc, argv, 0, 0, &arguments);

	watch_file = arguments.watch_path;

	settings = (struct parser_settings){ .max_parse_len = P_MAX_EXP_LEN,
					     .err_stream = err_stream };

	ectx.print_expr = false;
	ectx.err_stream = err_stream;
	ectx.out_stream = out_stream;
	ectx.pctx = parser_new(&settings);
	ectx.no_newline = arguments.no_newline;
	if (!ectx.pctx) {
		fprintf(err_stream, "Failed to create parser context");
		err = EINVAL;
		goto err;
	}

	// Create default cfg incase cfg files are missing
	ectx.cfg = config_new();
	if (!ectx.cfg) {
		fprintf(err_stream, "Failed to create config\n");
		err = ENOMEM;
		goto err;
	}

	while ((cfg_path = locate_next_config_file(arguments.config_file))) {
		err = config_overwrite_from_file(cfg_path, ectx.cfg);
		if (err) {
			if (err < 0) {
				fprintf(err_stream,
					"Unable to read config file %s: %s\n",
					cfg_path, strerror(-err));
				goto err;
			}

			fprintf(err_stream,
				"There was a config parse error at %s:%d. See bmath-config(5) for valid options\n",
				cfg_path, err);

			err = EINVAL;
			goto err;
		}
		free(cfg_path);
		cfg_path = NULL;
	}

	// When arguments have some cfg set, compare and replace with parsed cfg
	if (arguments.cfg_changed) {
		if (arguments.cfg_changed & CFG_FMT_ENCODINGS) {
			// need to free previous to write with new
			free(ectx.cfg->encoding_order);
			ectx.cfg->encoding_order =
				arguments.cfg->encoding_order;
			ectx.cfg->encoding_order_len =
				arguments.cfg->encoding_order_len;
			// set to null so we don't free the ptr swap
			arguments.cfg->encoding_order = NULL;
		}

		// TODO: This is kinda dumb how it is currently, but once --no-fmt-human, etc... are
		// implemented, then we need to know which changed and how to convert true -> false,
		// false -> true situations for only the changed options
		if (arguments.cfg_changed & CFG_FMT_HUMAN) {
			ectx.cfg->enc_fmt |= FMT_HUMAN;
		}

		if (arguments.cfg_changed & CFG_FMT_UPPERCASE) {
			ectx.cfg->enc_fmt |= FMT_UPPERCASE;
		}

		if (arguments.cfg_changed & CFG_FMT_JUTSIFY) {
			ectx.cfg->output_fmt |= OUT_FMT_JUSTIFY;
		}
	}
	config_free(arguments.cfg);
	arguments.cfg = NULL;

	if (arguments.watch) {
		if (!arguments.watch_path) {
			fprintf(err_stream, "Missing FILE for the -w option\n");
			err = EINVAL;
			goto err;
		}

		return do_watch(&ectx, arguments.watch_path);
	}

	if (arguments.headless) {
		err = evaluate(&ectx, arguments.headless,
			       strlen(arguments.headless));
		goto err;
	}

	if (isatty(0)) {
		return do_readline(&ectx);
	}

	return do_stdin(&ectx);

err:
	flush_streams(&ectx);
	if (cfg_path) {
		free(cfg_path);
	}
	config_free(arguments.cfg);
	execution_free(&ectx);
	return err;
}
