#pragma once

#include <stdint.h>
#include <asm-generic/errno-base.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "functions.h"

#define ATTR_LPAREN 1
#define ATTR_RPAREN ATTR_LPAREN + 1
#define ATTR_BITWISE_NOT ATTR_RPAREN + 1
#define ATTR_LSHIFT ATTR_BITWISE_NOT + 1
#define ATTR_RSHIFT ATTR_LSHIFT + 1
#define ATTR_OP_AND ATTR_RSHIFT + 1
#define ATTR_OP_OR ATTR_OP_AND + 1
#define ATTR_OP_XOR ATTR_OP_OR + 1
#define ATTR_FACTOR_OP_MUL ATTR_OP_XOR + 1
#define ATTR_FACTOR_OP_MOD ATTR_FACTOR_OP_MUL + 1
#define ATTR_SIGN_PLUS ATTR_FACTOR_OP_MOD + 1
#define ATTR_SIGN_MINUS ATTR_SIGN_PLUS + 1
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
	TOK_FUNCTION,
	TOK_COMMA,
};

static const char *lookup_token_name[] = {
	[TOK_NULL] = "null",	     [TOK_NUMBER] = "number",
	[TOK_OP] = "|, ^, or &",     [TOK_SHIFT_OP] = "<<, or >>",
	[TOK_LPAREN] = "(",	     [TOK_RPAREN] = ")",
	[TOK_BITWISE_NOT] = "~",     [TOK_SIGN] = "+, or -",
	[TOK_FACTOR_OP] = "*, or %", [TOK_FUNCTION] = "function",
	[TOK_COMMA] = ",",
};

struct token {
	uint64_t attr;
	size_t namelen;
	enum token_type type;
};

struct token_func {
	char *name;
	bmath_func_t func;
};

static inline const char *token_name(enum token_type tok)
{
	return lookup_token_name[tok];
}

static inline enum token_type token_type(struct token *tok)
{
	return tok->type;
}

struct token_tbl;

struct token_tbl *token_tbl_new();
void token_tbl_free(struct token_tbl *root);
int token_tbl_insert(struct token_tbl *tbl, const char *key, struct token);
struct token *token_tbl_lookup(struct token_tbl *tbl, const char *key);
int token_tbl_register_func(struct token_tbl *tbl, struct token_func *func);
