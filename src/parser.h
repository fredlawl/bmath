#pragma once

#include <stdint.h>
#include <stddef.h>

/*
 * Parse error codes
 */
#define PE_EXPRESSION_TOO_LONG 1
#define PE_PARSE_ERROR 2
#define PE_NOTHING_TO_PARSE 3

struct parser_context;

struct parser_settings {
	int max_parse_len;
};

struct parser_context *parser_new(struct parser_settings *settings);
int parser_free(struct parser_context *ctx);

/**
 * Convert infix notation to postfix notation. This takes care of parsing
 * operands and hex for operation.
 * @param const char *infix_expression
 * @param size_t len
 * @param uint64_t *out_result Result of the evaluation
 * @return Any positive integer means successful parse; a zero
 *         or negative value corresponds with a error code
 */
int parse(struct parser_context *ctx, const char *infix_expression, size_t len,
	  uint64_t *out_result);
