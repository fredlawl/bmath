#include <argp.h>
#include <sys/inotify.h>
#include <locale.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

static int read_file(struct parser_context *ctx, int fd, uint64_t alignment)
{
#define BUF_SIZE 4096
	int err;
	ssize_t bytes_read = 0;
	ssize_t expr_index = 0;
	char expr[P_MAX_EXP_LEN] = { 0 };

	do {
		char read_buff[BUF_SIZE] = { 0 };
		ssize_t buff_index = 0;

		bytes_read = read(fd, read_buff, sizeof(read_buff));
		if (bytes_read < 0) {
			perror("read_file() error reading line");
			return EINVAL;
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
	return 0;
}

static int do_stdin(struct parser_context *ctx, uint64_t alignment)
{
	int err;
	err = read_file(ctx, STDIN_FILENO, alignment);
	if (err) {
		parser_free(ctx);
		return EXIT_FAILURE;
	}

	parser_free(ctx);
	return EXIT_SUCCESS;
}

static void clear_screen()
{
	fputs("\033[2J\033[H", stdout);
}

static int handle_watch_event(struct parser_context *ctx, int notify_fd,
			      uint64_t alignment)
{
	ssize_t bytes_read;
	char buf[4096]
		__attribute__((aligned(__alignof__(struct inotify_event))));
	const struct inotify_event *event;

	while (1) {
		bytes_read = read(notify_fd, buf, sizeof(buf));
		if (bytes_read < 0 && errno != EAGAIN) {
			perror("read");
			break;
		}

		if (bytes_read <= 0) {
			fprintf(stderr, "loop broke\n");
			break;
		}

		for (char *ptr = buf; ptr < buf + bytes_read;
		     ptr += sizeof(struct inotify_event) + event->len) {
			event = (struct inotify_event *)ptr;

			if (event->mask & IN_CLOSE_WRITE) {
				fprintf(stderr, "close write\n");
				// goto out;
			}

			if (event->mask & IN_DELETE) {
				fprintf(stderr, "delete\n");
				// goto out;
			}

			if (event->mask & IN_DELETE_SELF) {
				fprintf(stderr, "delete self\n");
				// goto out;
			}

			if (event->mask & IN_CLOSE) {
				fprintf(stderr, "close\n");
				// goto out;
			}

			if (event->mask & IN_IGNORED) {
				fprintf(stderr, "need a rewatch\n");
				// goto out;
			}
			//clear_screen();
			fprintf(stderr, "%s: file got wrote\n", event->name);
		}
	}

	return 0;
}

static int do_watch(struct parser_context *ctx, const char *watch_file_path,
		    uint64_t alignment)
{
	int notify_fd, watch_fd, poll_num;
	int err;
	struct pollfd pollfds[1];
	int exit = EXIT_FAILURE;

	notify_fd = inotify_init();
	if (notify_fd < 0) {
		perror("do_watch(): couldn't inotify_init");
		return EXIT_FAILURE;
	}

	watch_fd = inotify_add_watch(notify_fd, watch_file_path, IN_ALL_EVENTS);
	if (watch_fd < 0) {
		perror("do_watch(): couldn't add watch");
		goto err_notify;
	}

	pollfds[0].fd = notify_fd;
	pollfds[0].events = POLLIN;

	while (1) {
		poll_num = poll(pollfds, 1, -1);
		if (poll_num == -1) {
			if (errno == EINTR)
				continue;
			perror("poll");
			goto err_watch;
		}

		if (poll_num > 0) {
			if (pollfds[0].revents & POLLIN) {
				err = handle_watch_event(ctx, notify_fd,
							 alignment);
				if (err) {
					goto err_watch;
				}
			}
		}
	}

	exit = EXIT_SUCCESS;
err_watch:
	inotify_rm_watch(notify_fd, watch_fd);
err_notify:
	close(notify_fd);
	parser_free(ctx);
	return exit;
}

int main(int argc, char *argv[])
{
	int err;
	struct parser_settings settings;
	struct parser_context *ctx;
	struct arguments arguments;
	char stdout_buff[4096] = { 0 };
	uint64_t alignment = 0;

	setvbuf(stdout, stdout_buff, _IOFBF, sizeof(stdout_buff));
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

	settings = (struct parser_settings){ .max_parse_len = P_MAX_EXP_LEN };

	ctx = parser_new(&settings);

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

	if (arguments.watch) {
		if (!arguments.watch_path) {
			fprintf(stderr, "Missing FILE for the -w option.\n");
			parser_free(ctx);
			return 1;
		}

		return do_watch(ctx, arguments.watch_path, alignment);
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
