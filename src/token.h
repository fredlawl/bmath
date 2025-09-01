#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdint.h>

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
	TOK_SIGN,
	TOK_FACTOR_OP,
	TOK_COMMA,
	TOK_ASSIGNMENT,
	TOK_IDENT,
	TOK_TERMINATOR
};

static const char *lookup_token_name[] = {
	[TOK_NULL] = "null",
	[TOK_NUMBER] = "number",
	[TOK_OP] = "|, ^, or &",
	[TOK_SHIFT_OP] = "<<, or >>",
	[TOK_LPAREN] = "(",
	[TOK_RPAREN] = ")",
	[TOK_BITWISE_NOT] = "~",
	[TOK_SIGN] = "+, or -",
	[TOK_FACTOR_OP] = "*, /, or %",
	[TOK_COMMA] = ",",
	[TOK_ASSIGNMENT] = "=",
	[TOK_IDENT] = "identfier",
	[TOK_TERMINATOR] = ";",
};

struct token {
	uint64_t attr;
	enum token_type type;
};

static inline const char *token_name(enum token_type tok)
{
	return lookup_token_name[tok];
}

static inline enum token_type token_type(struct token *tok)
{
	return tok->type;
}
