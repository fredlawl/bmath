#include <argp.h>
#include <asm-generic/errno-base.h>
#include <fcntl.h>
#include <poll.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <unistd.h>

// readline doesn't have FILE declared
#include <readline/history.h>
#include <readline/readline.h>

#include "config.h"
#include "parser.h"
#include "print.h"
#include "util.h"
#include "execute.h"

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

void flush_streams(struct execution_ctx *ectx)
{
	fflush(ectx->out_stream);
	fflush(ectx->err_stream);
}

void execution_free(struct execution_ctx *ectx)
{
	if (!ectx) {
		return;
	}

	config_free(ectx->cfg);
	parser_free(ectx->pctx);
	ectx = NULL;
}

static int _eval(struct execution_ctx *ectx,
		 const struct parse_expression *expr, uint64_t *out)
{
	int err;

	err = parse(ectx->pctx, expr->expr, expr->len, out);
	if (err) {
		switch (err) {
		case PE_NOTHING_TO_PARSE:
			// TODO: Add a means that when in watch or stream mode, this message
			// only shows if there's litterally nothing to display
			//fputs("Nothing to parse\n", ectx->err_stream);
			break;
		case PE_EXPRESSION_TOO_LONG:
			fputs("Expression too long\n", ectx->err_stream);
			break;
		case PE_PARSE_ERROR:
			fputs("Parse error ocurred\n", ectx->err_stream);
			break;
		default:
			fputs("Unknown error ocurred\n", ectx->err_stream);
		}
	}

	return err;
}

int evaluate(struct execution_ctx *ectx, const char *expr, size_t len)
{
	int err;
	uint64_t output = 0;
	ssize_t bytes_out;

	err = _eval(ectx, &(struct parse_expression){ expr, len }, &output);
	if (err) {
		flush_streams(ectx);
		return err;
	}

	if (ectx->print_expr) {
		if (ectx->count > 0) {
			fprintf(ectx->out_stream, "\n%s\n", expr);
		} else {
			fprintf(ectx->out_stream, "%s\n", expr);
		}
	}

	bytes_out = print_all(ectx->out_stream, output,
			      ectx->cfg->encoding_order,
			      ectx->cfg->encoding_order_len, ectx->cfg->enc_fmt,
			      ectx->cfg->output_fmt);

	if (bytes_out > 0) {
		fputc('\n', ectx->out_stream);
	}

	ectx->count++;

	flush_streams(ectx);
	return err;
}

int do_readline(struct execution_ctx *ectx)
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
		fputc('\n', ectx->out_stream);
		flush_streams(ectx);

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
			_perror(ectx->err_stream, "Unable to read input line");
			return EINVAL;
		}

		if (bytes_read == 0)
			break;

		while (buff_index < bytes_read) {
			if (unlikely(expr_index > P_MAX_EXP_LEN)) {
				fputs("Attempted input buffer overflow. Skipping",
				      ectx->err_stream);
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

int do_stdin(struct execution_ctx *ectx)
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

static void clear_screen(struct execution_ctx *ectx, const char *msg)
{
	int err;
	err = system("clear");
	if (err) {
		fputs("\033[2J\033[H", ectx->out_stream);
	}
	fputs(msg, ectx->out_stream);
	flush_streams(ectx);
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
			_perror(ectx->err_stream,
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
				clear_screen(ectx, "Push Ctrl-c to close\n");
				openfd = open(watch_path, O_CLOEXEC, O_RDONLY);
				if (openfd < 0) {
					_perror(ectx->err_stream,
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
					_perror(ectx->err_stream,
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

int do_watch(struct execution_ctx *ectx, const char *watch_file_path)
{
	int notify_fd, watch_fd, poll_num;
	int err;
	struct pollfd pollfd;
	int openfd;
	int exit = EXIT_FAILURE;

	notify_fd = inotify_init();
	if (notify_fd < 0) {
		_perror(ectx->err_stream, "Couldn't init inotifiy");
		return EXIT_FAILURE;
	}

	watch_fd = inotify_add_watch(notify_fd, watch_file_path, IN_IGNORED);
	if (watch_fd < 0) {
		_perror(ectx->err_stream, "Couldn't add watch for file \"%s\"",
			watch_file_path);
		goto err_notify;
	}

	pollfd.fd = notify_fd;
	pollfd.events = POLLIN;

	// on watch, immeditely display file contents
	clear_screen(ectx, "Push Ctrl-c to close\n");
	openfd = open(watch_file_path, O_CLOEXEC, O_RDONLY);
	if (openfd < 0) {
		_perror(ectx->err_stream, "Unable to open file \"%s\"",
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
			_perror(ectx->err_stream,
				"Polling for file events failed");
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
