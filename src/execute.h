#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#define P_MAX_EXP_LEN 512

struct parse_expression {
	const char *expr;
	size_t len;
};

struct execution_ctx {
	struct parser_context *pctx;
	struct config *cfg;
	bool print_expr;
	FILE *out_stream;
	FILE *err_stream;
	int count;
};

void execution_free(struct execution_ctx *ectx);
int evaluate(struct execution_ctx *ectx, const char *expr, size_t len);
int do_readline(struct execution_ctx *ectx);
int do_stdin(struct execution_ctx *ectx);
int do_watch(struct execution_ctx *ectx, const char *watch_file_path);
void flush_streams(struct execution_ctx *ectx);
