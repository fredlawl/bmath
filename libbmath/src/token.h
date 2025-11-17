#pragma once

#include "type.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdint.h>
#include <stdio.h>

#define ATTR_MAXCHARACTER_SET 127
#define ATTR_LSHIFT ATTR_MAXCHARACTER_SET + 1
#define ATTR_RSHIFT ATTR_LSHIFT + 1
#define ATTR_NULL UINT64_MAX

enum token_type {
	TOK_NULL = 0,
	TOK_NUMBER,
	TOK_OP,
	TOK_SHIFT_OP,
	TOK_LPAREN,
	TOK_RPAREN,
	TOK_BITWISE_NOT,
	TOK_ADDITIVE_OP,
	TOK_FACTOR_OP,
	TOK_COMMA,
	TOK_ASSIGNMENT,
	TOK_IDENT,
	TOK_TERMINATOR,
	TOK_VARIABLE,
	TOK_COMMENT,
};

static const char *lookup_token_name[] = {
	[TOK_NULL] = "null",
	[TOK_NUMBER] = "number",
	[TOK_OP] = "|, ^, or &",
	[TOK_SHIFT_OP] = "<<, or >>",
	[TOK_LPAREN] = "(",
	[TOK_RPAREN] = ")",
	[TOK_BITWISE_NOT] = "~",
	[TOK_ADDITIVE_OP] = "+, or -",
	[TOK_FACTOR_OP] = "*, /, or %",
	[TOK_COMMA] = ",",
	[TOK_ASSIGNMENT] = "=",
	[TOK_IDENT] = "identifier",
	[TOK_TERMINATOR] = ";",
	[TOK_VARIABLE] = "@",
	[TOK_COMMENT] = "comment",
};

struct token {
	union {
		uint64_t attr;
		bmath_result_t ret;
	} d;
#define tok_attr d.attr
#define tok_ret d.ret
	size_t line;
	size_t offset;
	size_t len;
	enum token_type type;
};

static inline const char *token_name(enum token_type tok)
{
	return lookup_token_name[tok];
}

static inline enum token_type token_type(const struct token *tok)
{
	return tok->type;
}

static inline int token_cmp(const struct token *a, const struct token *b)
{
	return !(a->type == b->type && a->line == b->line &&
		 a->offset == b->offset && a->len == b->len);
}

static inline size_t token_str(const struct token *token, char *buffer,
			       size_t len)
{
	return snprintf(buffer, len,
			"Token(type=%s, line=%lu, offset=%lu, len=%lu)",
			token_name(token_type(token)), token->line,
			token->offset, token->len);
}
