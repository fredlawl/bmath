#include <argp.h>
#include <fcntl.h>
#include <locale.h>
#include <poll.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <unistd.h>

// readline doesn't have FILE declared
#include <readline/history.h>
#include <readline/readline.h>

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
static char *watch_file = NULL;

static FILE *err_stream;
static FILE *out_stream;

#define P_MAX_EXP_LEN 512

struct parse_expression {
	const char *expr;
	size_t len;
};

struct execution_ctx {
	struct parser_context *pctx;
	uint64_t alignment;
	bool print_expr;
};

static void _perror(FILE *stream, const char *fmt, ...)
{
	int err = errno;
	va_list args;
	va_start(args, fmt);
	fprintf(stream, fmt, args);
	va_end(args);
	fprintf(stream, ": %s\n", strerror(err));
	// incase fprintf overwrites this, put it back
	errno = err;
}

static void flush_streams()
{
	fflush(out_stream);
	fflush(err_stream);
}

void execution_free(struct execution_ctx *ectx)
{
	if (ectx->pctx) {
		parser_free(ectx->pctx);
		ectx->pctx = NULL;
	}
}

static int _eval(struct parser_context *ctx,
		 const struct parse_expression *expr, uint64_t *out)
{
	int err;

	err = parse(ctx, expr->expr, expr->len, out);
	if (err) {
		switch (err) {
		case PE_NOTHING_TO_PARSE:
			// TODO: Add a means that when in watch or stream mode, this message
			// only shows if there's litterally nothing to display
			fputs("Nothing to parse.\n", err_stream);
			break;
		case PE_EXPRESSION_TOO_LONG:
			fputs("Expression too long.\n", err_stream);
			break;
		case PE_PARSE_ERROR:
			fputs("Parse error ocurred.\n", err_stream);
			break;
		default:
			fputs("Unknown error ocurred.\n", err_stream);
		}
	}

	return err;
}

static int evaluate(struct execution_ctx *ectx, const char *expr, size_t len)
{
	int err;
	uint64_t output = 0;

	err = _eval(ectx->pctx, &(struct parse_expression){ expr, len },
		    &output);
	if (err) {
		fputc('\n', err_stream);
		flush_streams();
		return err;
	}

	if (ectx->print_expr) {
		fprintf(out_stream, "%s\n", expr);
	}

	print_set_stream(out_stream);
	print_number(output, uppercase_hex,
		     (show_unicode) ? ENC_ALL : ENC_ASCII);

	if (ectx->alignment) {
		print_alignment(ectx->alignment, output, uppercase_hex);
	}

	if (show_binary) {
		print_binary(output);
	}

	fputc('\n', out_stream);
	flush_streams();
	return err;
}

static int do_readline(struct execution_ctx *ectx)
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
		evaluate(ectx, input, strlen(input));
		flush_streams();

		free(input);
	}

	free(input);
	execution_free(ectx);
	return EXIT_SUCCESS;
}

static int read_file(struct execution_ctx *ectx, int fd)
{
#define BUF_SIZE 4096
	ssize_t bytes_read = 0;
	ssize_t expr_index = 0;
	char expr[P_MAX_EXP_LEN] = { 0 };

	do {
		char read_buff[BUF_SIZE] = { 0 };
		ssize_t buff_index = 0;

		bytes_read = read(fd, read_buff, sizeof(read_buff));
		if (bytes_read < 0) {
			_perror(err_stream, "Unable to read input line");
			return EINVAL;
		}

		if (bytes_read == 0)
			break;

		while (buff_index < bytes_read) {
			if (unlikely(expr_index > P_MAX_EXP_LEN)) {
				fputs("Attempted input buffer overflow. Skipping.\n",
				      err_stream);
				memset(expr, '\0', P_MAX_EXP_LEN);
				expr_index = 0;
				while (buff_index++ < bytes_read &&
				       read_buff[buff_index] != '\n')
					;
				buff_index++;
				continue;
			}

			if (read_buff[buff_index] == '\n') {
				ectx->print_expr = true;
				// ignore error handling for evaluate to keep program running
				evaluate(ectx, expr, expr_index);
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

	return 0;
}

static int do_stdin(struct execution_ctx *ectx)
{
	int err;
	err = read_file(ectx, STDIN_FILENO);
	if (err) {
		execution_free(ectx);
		return EXIT_FAILURE;
	}

	execution_free(ectx);
	return EXIT_SUCCESS;
}

static void clear_screen(const char *msg)
{
	int err;
	err = system("clear");
	if (err) {
		fputs("\033[2J\033[H", out_stream);
	}
	fputs(msg, out_stream);
	flush_streams();
}

static int handle_watch_event(struct execution_ctx *ectx,
			      const char *watch_path, int notify_fd,
			      int *watch_fd)
{
	int openfd;
	ssize_t bytes_read;
	char buf[4096]
		__attribute__((aligned(__alignof__(struct inotify_event))));
	const struct inotify_event *event;

	while (1) {
		bytes_read = read(notify_fd, buf, sizeof(buf));
		if (bytes_read < 0 && errno != EAGAIN) {
			_perror(err_stream,
				"Couldn't read the notify events buffer");
			break;
		}

		if (bytes_read <= 0) {
			break;
		}

		for (char *ptr = buf; ptr < buf + bytes_read;
		     ptr += sizeof(struct inotify_event) + event->len) {
			event = (struct inotify_event *)ptr;

			/*
       * IN_IGNORE is when the inode is changed in some way. When this happens
       * we must re-attach the notify to the file's new inode.
       * When that happens, then perform a print.
       */
			if (event->mask & IN_IGNORED) {
				clear_screen("Push Ctrl-c to close\n");
				openfd = open(watch_path, O_CLOEXEC, O_RDONLY);
				if (openfd < 0) {
					_perror(err_stream,
						"Unable to open file \"%s\"",
						watch_path);
					return errno;
				}

				int err = read_file(ectx, openfd);
				if (err) {
					close(openfd);
					return EINVAL;
				}
				close(openfd);

				*watch_fd = inotify_add_watch(
					notify_fd, watch_path, IN_IGNORED);
				if (*watch_fd < 0) {
					_perror(err_stream,
						"Couldn't add watch for file \"%s\"",
						watch_path);
					return EINVAL;
				}

				continue;
			}
		}
	}

	return 0;
}

static int do_watch(struct execution_ctx *ectx, const char *watch_file_path)
{
	int notify_fd, watch_fd, poll_num;
	int err;
	struct pollfd pollfd;
	int openfd;
	int exit = EXIT_FAILURE;

	notify_fd = inotify_init();
	if (notify_fd < 0) {
		_perror(err_stream, "Couldn't init inotifiy");
		return EXIT_FAILURE;
	}

	watch_fd = inotify_add_watch(notify_fd, watch_file_path, IN_IGNORED);
	if (watch_fd < 0) {
		_perror(err_stream, "Couldn't add watch for file \"%s\"",
			watch_file_path);
		goto err_notify;
	}

	pollfd.fd = notify_fd;
	pollfd.events = POLLIN;

	// on watch, immeditely display file contents
	clear_screen("Push Ctrl-c to close\n");
	openfd = open(watch_file_path, O_CLOEXEC, O_RDONLY);
	if (openfd < 0) {
		_perror(err_stream, "Unable to open file \"%s\"",
			watch_file_path);
		return errno;
	}

	err = read_file(ectx, openfd);
	if (err) {
		close(openfd);
		goto err_watch;
	}
	close(openfd);

	while (1) {
		poll_num = poll(&pollfd, 1, -1);
		if (poll_num == -1) {
			if (errno == EINTR)
				continue;
			_perror(err_stream, "Polling for file events failed");
			goto err_watch;
		}

		if (poll_num > 0) {
			if (pollfd.revents & POLLIN) {
				err = handle_watch_event(ectx, watch_file_path,
							 notify_fd, &watch_fd);
				if (err) {
					goto err_watch;
				}
			}
		}
	}

	exit = EXIT_SUCCESS;
err_watch:
	// ignore error. program is exiting anyway
	inotify_rm_watch(notify_fd, watch_fd);
err_notify:
	close(notify_fd);
	execution_free(ectx);
	return exit;
}

int main(int argc, char *argv[])
{
	int err;
	struct parser_settings settings;
	struct arguments arguments;
	struct execution_ctx ectx = { 0 };
	char stdout_buff[4096] = { 0 };

	err_stream = stderr;
	out_stream = stdout;

	setvbuf(out_stream, stdout_buff, _IOFBF, sizeof(stdout_buff));
	setlocale(LC_CTYPE, "en_US.UTF-8");

	arguments.detached_expr = NULL;
	arguments.should_uppercase_hex = false;
	arguments.should_show_unicode = false;
	arguments.print_binary = false;
	arguments.alignment_expr = NULL;
	arguments.watch = false;
	arguments.watch_path = NULL;

	argp_parse(&argp, argc, argv, 0, 0, &arguments);

	uppercase_hex = arguments.should_uppercase_hex;
	show_unicode = arguments.should_show_unicode;
	show_binary = arguments.print_binary;
	watch_file = arguments.watch_path;

	settings = (struct parser_settings){ .max_parse_len = P_MAX_EXP_LEN,
					     .err_stream = err_stream };

	ectx.pctx = parser_new(&settings);
	if (!ectx.pctx) {
		fprintf(err_stream, "Failed to create parser context");
		return EXIT_FAILURE;
	}

	ectx.print_expr = false;

	if (arguments.alignment_expr) {
		err = _eval(ectx.pctx,
			    &(struct parse_expression){
				    arguments.alignment_expr,
				    strlen(arguments.alignment_expr) },
			    &ectx.alignment);
		if (err) {
			fprintf(err_stream,
				"Unable to parse the align expression.");
			flush_streams();
			execution_free(&ectx);
			return err;
		}
	}

	if (arguments.watch) {
		if (!arguments.watch_path) {
			fprintf(err_stream,
				"Missing FILE for the -w option.\n");
			execution_free(&ectx);
			return 1;
		}

		return do_watch(&ectx, arguments.watch_path);
	}

	if (arguments.detached_expr) {
		err = evaluate(&ectx, arguments.detached_expr,
			       strlen(arguments.detached_expr));
		flush_streams();
		execution_free(&ectx);
		return err;
	}

	if (isatty(0)) {
		return do_readline(&ectx);
	}

	return do_stdin(&ectx);
}
