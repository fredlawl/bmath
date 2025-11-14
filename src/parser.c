#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "functions.h"
#include "lexer.h"
#include "parser.h"
#include "token.h"
#include "symbol.h"

struct parser_context {
	int max_parse_len;
	FILE *err_stream;
	struct symbol_tbl *tbl;
	struct lexer *lexer;
	struct token lookahead_token;
};

static void __expect(struct parser_context *pctx, enum token_type expected);

static uint64_t expr_number(struct parser_context *pctx);
static uint64_t expr_function(struct parser_context *pctx);
static uint64_t expr_signed(struct parser_context *pctx);
static uint64_t expr_factor(struct parser_context *pctx);
static uint64_t expr_add(struct parser_context *pctx);
static uint64_t expr_shift(struct parser_context *pctx);
static uint64_t expr_and(struct parser_context *pctx);
static uint64_t expr_xor(struct parser_context *pctx);
static uint64_t expr_or(struct parser_context *pctx);
static uint64_t expr_assignment(struct parser_context *pctx);
static uint64_t expr(struct parser_context *pctx);

static uint64_t __perform_parse(struct parser_context *pctx)
{
	pctx->lookahead_token = lexer_next_token(pctx->lexer);
	return expr(pctx);
}

struct parser_context *parser_new(struct parser_settings *settings)
{
	struct lexer_settings lexer_settings = { 0 };
	struct symbol_table_attr tbl_attr;
	struct parser_context *ctx = malloc(sizeof(*ctx));
	if (!ctx) {
		return NULL;
	}

	tbl_attr.key_size = 32;

	ctx->tbl = symbol_table_new(&tbl_attr);
	if (!ctx->tbl) {
		goto out_err;
	}

	ctx->max_parse_len = settings->max_parse_len;
	ctx->err_stream = stderr;
	if (settings->err_stream) {
		ctx->err_stream = settings->err_stream;
	}

	lexer_settings.tbl = ctx->tbl;
	lexer_settings.err_stream = ctx->err_stream;
	ctx->lexer = lexer_new(&lexer_settings);
	if (!ctx->lexer) {
		goto out_err;
	}

	return ctx;

out_err:
	parser_free(ctx);
	return NULL;
}

int parser_free(struct parser_context *ctx)
{
	if (!ctx) {
		return 0;
	}

	lexer_free(ctx->lexer);
	symbol_table_free(ctx->tbl);
	free(ctx);
	return 0;
}

int parse(struct parser_context *ctx, const char *infix_expression, size_t len,
	  uint64_t *out_result)
{
	uint64_t result;

	*out_result = 0;

	if (len == 0)
		return PE_NOTHING_TO_PARSE;

	if (len > (size_t)ctx->max_parse_len)
		return PE_EXPRESSION_TOO_LONG;

	lexer_init(ctx->lexer, infix_expression, (int16_t)len);
	result = __perform_parse(ctx);

	if (lexer_in_error(ctx->lexer)) {
		return PE_PARSE_ERROR;
	}

	*out_result = result;

	return 0;
}

static void __expect(struct parser_context *pctx, enum token_type expected)
{
	if (pctx->lookahead_token.type == expected) {
		pctx->lookahead_token = lexer_next_token(pctx->lexer);
		return;
	}

	if (!lexer_in_error(pctx->lexer)) {
		lexer_lexical_error(pctx->lexer,
				    "Expecting a %s, but got %s instead.",
				    token_name(expected),
				    token_name(pctx->lookahead_token.type));
	}
}

static uint64_t expr_number(struct parser_context *pctx)
{
	uint64_t ret;

	if (pctx->lookahead_token.type == TOK_LPAREN) {
		__expect(pctx, TOK_LPAREN);
		ret = expr(pctx);
		__expect(pctx, TOK_RPAREN);
		return ret;
	}

	if (pctx->lookahead_token.type == TOK_IDENT) {
		ret = *(uint64_t *)symbol_value(
			(struct symbol *)pctx->lookahead_token.attr);
		__expect(pctx, TOK_IDENT);
	} else {
		ret = pctx->lookahead_token.attr;
		__expect(pctx, TOK_NUMBER);
	}
	return ret;
}

static uint64_t expr_function(struct parser_context *pctx)
{
	int err = 0;
	static uint64_t ops[FUNCTIONS_MAX_OPS] = { 0 };
	uint64_t ret = 0;
	size_t i;
	struct symbol *sym;
	bmath_func_t func;
	struct token tok;

	if (pctx->lookahead_token.type != TOK_IDENT) {
		return expr_number(pctx);
	}

	memset(ops, 0, sizeof(ops));

	tok = pctx->lookahead_token;
	sym = (struct symbol *)tok.attr;
	if (sym->type != SYMBOL_FUNCTION) {
		return expr_number(pctx);
	}

	func = (bmath_func_t) * (uintptr_t *)symbol_value(sym);

	__expect(pctx, TOK_IDENT);
	__expect(pctx, TOK_LPAREN);
	for (i = 0; i < sizeof(ops) / sizeof(ops[0]); i++) {
		if (pctx->lookahead_token.type == TOK_RPAREN) {
			--i;
			break;
		}

		ops[i] = expr(pctx);
		if (pctx->lookahead_token.type != TOK_COMMA) {
			break;
		}
		__expect(pctx, TOK_COMMA);
	}
	__expect(pctx, TOK_RPAREN);

	err = func(&ret, i + 1, ops);
	if (err) {
		lexer_lexical_error(pctx->lexer,
				    "%s() returned error code: %d %s",
				    symbol_ident(sym), err, str_func_err(err));
		return ret;
	}

	return ret;
}

static uint64_t expr_signed(struct parser_context *pctx)
{
#define MAX_STACK 10
	static struct token stack[MAX_STACK] = { 0 };
	struct token tok;

	uint64_t ret;
	int i = -1;
	int in_loop = 0;

	while (1) {
		tok = pctx->lookahead_token;
		switch (tok.type) {
		case TOK_BITWISE_NOT:
		case TOK_SIGN:
			if (!in_loop) {
				in_loop = 1;
				memset(stack, 0, sizeof(stack));
			}
			i++;
			if (i >= MAX_STACK) {
				lexer_lexical_error(
					pctx->lexer,
					"Exceeded max stack depth of %d",
					MAX_STACK);
				return 0;
			}
			stack[i] = tok;
			__expect(pctx, pctx->lookahead_token.type);
			break;
		default:
			ret = expr_function(pctx);
			goto next;
		}
	}

next:
	for (int j = i; j >= 0; j--) {
		switch (stack[j].type) {
		case TOK_BITWISE_NOT:
			ret = ~ret;
			break;
		case TOK_SIGN:
			if (stack[j].attr == '-') {
				ret = -ret;
			}
			break;
		default:
			break;
		}
	}

	return ret;
}

static uint64_t expr_factor(struct parser_context *pctx)
{
	uint64_t left, right;
	struct token tok;

	left = expr_signed(pctx);
	while (true) {
		if (pctx->lookahead_token.type != TOK_FACTOR_OP) {
			break;
		}

		tok = pctx->lookahead_token;
		__expect(pctx, pctx->lookahead_token.type);
		right = expr_signed(pctx);
		switch (tok.attr) {
		case '*':
			left *= right;
			break;
		case '/':
			if (right == 0) {
				lexer_lexical_error(pctx->lexer,
						    "Division by zero");
				return left;
			}
			left /= right;
			break;
		case '%':
			if (right == 0) {
				lexer_lexical_error(pctx->lexer,
						    "Division by zero");
				return left;
			}
			left %= right;
			break;
		default:
			lexer_general_error(
				pctx->lexer,
				"Something went wrong parsing term.\n");
		}
	}

	return left;
}

static uint64_t expr_add(struct parser_context *pctx)
{
	uint64_t left, right;
	struct token tok;

	left = expr_factor(pctx);
	while (true) {
		if (pctx->lookahead_token.type != TOK_SIGN) {
			break;
		}

		tok = pctx->lookahead_token;
		__expect(pctx, pctx->lookahead_token.type);
		right = expr_factor(pctx);
		switch (tok.attr) {
		case '+':
			left += right;
			break;
		case '-':
			left -= right;
			break;
		default:
			lexer_general_error(
				pctx->lexer,
				"Something went wrong parsing term.\n");
		}
	}

	return left;
}

static uint64_t expr_shift(struct parser_context *pctx)
{
	uint64_t left, right;
	struct token tok;

	left = expr_add(pctx);
	while (true) {
		if (pctx->lookahead_token.type != TOK_SHIFT_OP) {
			break;
		}

		tok = pctx->lookahead_token;
		__expect(pctx, pctx->lookahead_token.type);
		right = expr_add(pctx);
		switch (tok.attr) {
		case ATTR_LSHIFT:
			left <<= right;
			break;
		case ATTR_RSHIFT:
			left >>= right;
			break;
		default:
			lexer_general_error(
				pctx->lexer,
				"Something went wrong parsing term.\n");
		}
	}
	return left;
}

static uint64_t expr_and(struct parser_context *pctx)
{
	uint64_t left;

	left = expr_shift(pctx);
	if (pctx->lookahead_token.attr != '&')
		return left;

	__expect(pctx, TOK_OP);
	return left & expr_and(pctx);
}

static uint64_t expr_xor(struct parser_context *pctx)
{
	uint64_t left;

	left = expr_and(pctx);
	if (pctx->lookahead_token.attr != '^')
		return left;

	__expect(pctx, TOK_OP);
	return left ^ expr_xor(pctx);
}

static uint64_t expr_or(struct parser_context *pctx)
{
	uint64_t left;

	left = expr_xor(pctx);
	if (pctx->lookahead_token.attr != '|')
		return left;

	__expect(pctx, TOK_OP);
	return left | expr_or(pctx);
}

static uint64_t expr_assignment(struct parser_context *pctx)
{
	uint64_t ret;
	struct token ident;
	struct symbol *sym;

	ident = pctx->lookahead_token;
	ret = expr_or(pctx);

	if (ident.type != TOK_IDENT) {
		return ret;
	}

	if (!ident.attr) {
		lexer_general_error(
			pctx->lexer,
			"Token doesn't have a symbol. This should not happen");
		return 0;
	}

	sym = (struct symbol *)ident.attr;
	if (sym->type != SYMBOL_VARIABLE) {
		return ret;
	}

	if (pctx->lookahead_token.type != TOK_ASSIGNMENT) {
		return ret;
	}

	__expect(pctx, TOK_ASSIGNMENT);
	ret = expr(pctx);
	__expect(pctx, TOK_TERMINATOR);

	memcpy(symbol_value(sym), (void *)&ret, sizeof(ret));

	if (pctx->lookahead_token.type == TOK_NULL) {
		return ret;
	}

	return expr(pctx);
}

static uint64_t expr(struct parser_context *pctx)
{
	return expr_assignment(pctx);
}
