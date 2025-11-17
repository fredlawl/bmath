#include "type.h"
#include <execinfo.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "functions.h"
#include "lexer.h"
#include "parser.h"
#include "token.h"
#include "symbol.h"
#include "util.h"

static struct named_function {
	const char *name;
	size_t namelen;
	bmath_func_t func;
} PREDEFINED_FUNCTIONS[] = {
	{ "align", sizeof("algin") - 1, align },
	{ "align_down", sizeof("align_down") - 1, align_down },
	{ "bswap", sizeof("bswap") - 1, bswap },
	{ "clz", sizeof("clz") - 1, clz },
	{ "ctz", sizeof("ctz") - 1, ctz },
	{ "mask", sizeof("mask") - 1, mask },
	{ "popcnt", sizeof("popcnt") - 1, popcnt },
};

static inline struct symbol *named_func_to_sym(struct named_function *nfunc)
{
	uintptr_t value = (uintptr_t)nfunc->func;
	return symbol_new(nfunc->name, nfunc->namelen, SYMBOL_FUNCTION,
			  SYM_FLAG_DEFINED, &value, sizeof(value));
}

struct parser_context {
	int max_parse_len;
	FILE *err_stream;
	struct symbol_tbl *tbl;
	struct lexer *lexer;
	struct token lookahead_token;
	int in_error;
	const char *parsing;
};

static void __expect(struct parser_context *pctx, enum token_type expected);

static void parser_general_error(struct parser_context *pctx, const char *fmt,
				 ...)
{
	va_list args;

	fprintf(pctx->err_stream, "[ERROR]: ");

	va_start(args, fmt);
	vfprintf(pctx->err_stream, fmt, args);
	va_end(args);

	putc('\n', pctx->err_stream);
	pctx->in_error = PE_PARSE_ERROR;
}

static void parser_lexical_error_token(struct parser_context *pctx,
				       const struct token *token,
				       const char *fmt, ...)
{
	va_list args;

	if (pctx->in_error)
		return;

	fprintf(pctx->err_stream,
		"[PARSE ERROR]: There was an error parsing the expression:\n");
	fprintf(pctx->err_stream, "%s\n", pctx->parsing);
	__repeat_character(pctx->err_stream, (size_t)token->offset, '~');
	fprintf(pctx->err_stream, "^ ");
	va_start(args, fmt);
	vfprintf(pctx->err_stream, fmt, args);
	va_end(args);
	putc('\n', pctx->err_stream);

	pctx->in_error = PE_PARSE_ERROR;
}

#define parser_lexical_error(pctx, fmt, ...)                                  \
	do {                                                                  \
		parser_lexical_error_token(pctx, &pctx->lookahead_token, fmt, \
					   ##__VA_ARGS__);                    \
	} while (0);

static bmath_result_t expr_or(struct parser_context *pctx);
static bmath_result_t expr(struct parser_context *pctx);

static struct symbol *parser_token_to_symbol(struct parser_context *pctx,
					     const struct token *token)
{
	const char *token_ident;
	size_t ident_len;

	if (token->type != TOK_VARIABLE && token->type != TOK_IDENT) {
		return NULL;
	}

	token_ident = lexer_token_ident(pctx->lexer, token);
	if (!token_ident) {
		parser_lexical_error_token(pctx, token, "null identifier");
		return NULL;
	}

	ident_len = strlen(token_ident);
	if (ident_len > 31) {
		parser_lexical_error_token(
			pctx, token,
			"identifier too long. max %d characters got %d", 31,
			(int)ident_len);
		return NULL;
	}

	if (ident_len < 2 && token->type == TOK_VARIABLE) {
		parser_lexical_error_token(
			pctx, token,
			"identifier too short. min %d characters got %d", 2,
			(int)ident_len);
		return NULL;
	}

	return symbol_table_lookup(pctx->tbl, token_ident, ident_len);
}

#define bt()                                                     \
	do {                                                     \
		void *buf[100];                                  \
		size_t nptrs;                                    \
		nptrs = backtrace(buf, 100);                     \
		backtrace_symbols_fd(buf, nptrs, STDERR_FILENO); \
	} while (0);

#define print_token(tok)                                       \
	do {                                                   \
		char tok_str[256] = {};                        \
		token_str(&tok, tok_str, sizeof(tok_str) - 1); \
		fprintf(stderr, "token: %s\n", tok_str);       \
		bt();                                          \
	} while (0);

static struct token parser_next_token(struct parser_context *pctx)
{
	int lexer_err;
	struct token tok;
	struct symbol *sym;
	bmath_result_t temp = 0;
	const char *token_ident;
	size_t ident_len;
	int err;

	tok = lexer_next_token(pctx->lexer);
	lexer_err = lexer_errno(pctx->lexer);
	if (lexer_err && lexer_err != EOF) {
		parser_lexical_error(pctx, lexer_error_str(pctx->lexer));
		return tok;
	}

	if (tok.type != TOK_VARIABLE) {
		return tok;
	}

	token_ident = lexer_token_ident(pctx->lexer, &tok);
	if (!token_ident) {
		parser_lexical_error(pctx, "null identifier");
		return tok;
	}

	ident_len = strlen(token_ident);

	sym = parser_token_to_symbol(pctx, &tok);
	if (!pctx->in_error &&
	    (!sym || sym->ident_len != ident_len ||
	     strncmp(token_ident, symbol_ident(sym), ident_len))) {
		sym = symbol_new(token_ident, ident_len, SYMBOL_VARIABLE,
				 SYM_FLAG_NONE, &temp, sizeof(temp));
		if (!sym) {
			parser_general_error(pctx, "unable to allocate symbol");
			return tok;
		}

		err = symbol_table_update(pctx->tbl, sym);
		if (err) {
			parser_general_error(
				pctx, "unable add symbol to table: %d", err);
			return tok;
		}
	}

	return tok;
}

static bmath_result_t __perform_parse(struct parser_context *pctx)
{
	pctx->lookahead_token = parser_next_token(pctx);
	if (pctx->in_error) {
		return 0;
	}

	if (pctx->lookahead_token.type == TOK_COMMENT) {
		pctx->in_error = PE_NOTHING_TO_PARSE;
		return 0;
	}

	return expr(pctx);
}

struct parser_context *parser_new(struct parser_settings *settings)
{
	struct lexer_settings lexer_settings = {};
	struct symbol_table_attr tbl_attr;
	struct parser_context *ctx;
	int err;

	ctx = calloc(1, sizeof(*ctx));
	if (!ctx) {
		return NULL;
	}

	tbl_attr.key_size = 32;

	ctx->tbl = symbol_table_new(&tbl_attr);
	if (!ctx->tbl) {
		goto out_err;
	}

	for (size_t i = 0;
	     i < sizeof(PREDEFINED_FUNCTIONS) / sizeof(PREDEFINED_FUNCTIONS[0]);
	     i++) {
		struct symbol *sym =
			named_func_to_sym(&PREDEFINED_FUNCTIONS[i]);

		if (!sym) {
			continue;
		}

		err = symbol_table_update(ctx->tbl, sym);
		if (err) {
			symbol_free(sym);
		}
	}

	ctx->max_parse_len = settings->max_parse_len;
	ctx->err_stream = stderr;
	if (settings->err_stream) {
		ctx->err_stream = settings->err_stream;
	}

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
	  bmath_result_t *out_result)
{
	bmath_result_t result;
	int err;

	*out_result = 0;

	if (len == 0)
		return PE_NOTHING_TO_PARSE;

	if (len > (size_t)ctx->max_parse_len)
		return PE_EXPRESSION_TOO_LONG;

	lexer_init(ctx->lexer, infix_expression, len);
	ctx->parsing = infix_expression;
	result = __perform_parse(ctx);

	/*
	 * A valid parse must consume the entire token stream.
	 * If anything remains, report it as a parse error.
	 */
	err = ctx->in_error;
	if (!err && ctx->lookahead_token.type != TOK_COMMENT &&
	    ctx->lookahead_token.type != TOK_NULL) {
		parser_lexical_error_token(
			ctx, &ctx->lookahead_token,
			"unexpected token after expression: %s",
			token_name(ctx->lookahead_token.type));
	}

	err = ctx->in_error;
	if (err) {
		// reset error state
		ctx->in_error = 0;
		return err;
	}

	*out_result = result;
	return 0;
}

static void __expect(struct parser_context *pctx, enum token_type expected)
{
	if (pctx->lookahead_token.type == expected) {
		pctx->lookahead_token = parser_next_token(pctx);
		return;
	}

	parser_lexical_error(pctx, "expecting a %s, but got %s instead.",
			     token_name(expected),
			     token_name(pctx->lookahead_token.type));
}

static bmath_result_t expr_number(struct parser_context *pctx)
{
	bmath_result_t ret;
	struct token tok;
	struct symbol *sym;

	if (pctx->lookahead_token.type == TOK_LPAREN) {
		__expect(pctx, TOK_LPAREN);
		ret = expr_or(pctx);
		__expect(pctx, TOK_RPAREN);
		return ret;
	}

	if (pctx->lookahead_token.type == TOK_VARIABLE) {
		tok = pctx->lookahead_token;
		sym = parser_token_to_symbol(pctx, &tok);
		if (!sym) {
			parser_lexical_error_token(pctx, &tok,
						   "undefined variable");
			return 0;
		}

		ret = *(bmath_result_t *)symbol_value(sym);
		__expect(pctx, TOK_VARIABLE);
		if (!(sym->flags & SYM_FLAG_DEFINED) &&
		    pctx->lookahead_token.type != TOK_ASSIGNMENT) {
			parser_lexical_error_token(pctx, &tok,
						   "variable undefined");
		}
	} else {
		ret = pctx->lookahead_token.tok_ret;
		__expect(pctx, TOK_NUMBER);
	}

	return ret;
}

static bmath_result_t expr_function(struct parser_context *pctx)
{
	int err = 0;
	static bmath_result_t ops[FUNCTIONS_MAX_OPS] = { 0 };
	bmath_result_t ret = 0;
	size_t i;
	struct symbol *sym;
	bmath_func_t func;
	struct token tok;

	if (pctx->lookahead_token.type != TOK_IDENT) {
		return expr_number(pctx);
	}

	memset(ops, 0, sizeof(ops));

	tok = pctx->lookahead_token;
	sym = parser_token_to_symbol(pctx, &tok);
	if (!sym || !(sym->flags & SYM_FLAG_DEFINED)) {
		parser_lexical_error(pctx, "identifier undefined");
		return 0;
	}

	if (sym->type != SYMBOL_FUNCTION) {
		parser_lexical_error(pctx,
				     "expected function, got something else");
		return 0;
	}

	func = (bmath_func_t) * (uintptr_t *)symbol_value(sym);

	__expect(pctx, TOK_IDENT);
	__expect(pctx, TOK_LPAREN);
	for (i = 0; i < sizeof(ops) / sizeof(ops[0]); i++) {
		if (pctx->lookahead_token.type == TOK_RPAREN) {
			--i;
			break;
		}

		ops[i] = expr_or(pctx);
		if (pctx->lookahead_token.type != TOK_COMMA) {
			break;
		}
		__expect(pctx, TOK_COMMA);
	}
	__expect(pctx, TOK_RPAREN);

	err = func(&ret, i + 1, ops);
	if (err) {
		parser_lexical_error(pctx, "%s() returned error code: %d %s",
				     symbol_ident(sym), err, str_func_err(err));
		return ret;
	}

	return ret;
}

#define MAX_STACK 10

static bmath_result_t expr_signed(struct parser_context *pctx, int stack,
				  struct token tstack[])
{
	bmath_result_t ret;

	if (pctx->lookahead_token.type == TOK_ADDITIVE_OP ||
	    pctx->lookahead_token.type == TOK_BITWISE_NOT) {
		if (stack >= MAX_STACK) {
			parser_lexical_error(pctx,
					     "exceeded max stack depth of %d",
					     MAX_STACK);
			return 0;
		}

		tstack[stack] = pctx->lookahead_token;
		__expect(pctx, pctx->lookahead_token.type);
		return expr_signed(pctx, stack + 1, tstack);
	}

	ret = expr_function(pctx);
	for (int j = stack - 1; j >= 0; j--) {
		switch (tstack[j].tok_attr) {
		case '~':
			ret = bmath_result_t__complement(ret);
			break;
		case '-':
			ret = bmath_result_t__negate(ret);
			break;
		default:
			break;
		}
	}
	return ret;
}

static bmath_result_t expr_factor(struct parser_context *pctx)
{
	bmath_result_t left, right;
	struct token tok;
	struct token tstack[MAX_STACK];

	left = expr_signed(pctx, 0, tstack);
	while (pctx->lookahead_token.type == TOK_FACTOR_OP) {
		tok = pctx->lookahead_token;
		__expect(pctx, tok.type);
		right = expr_signed(pctx, 0, tstack);
		switch (tok.tok_attr) {
		case '*':
			left = bmath_result_t__mul(left, right);
			break;
		case '/':
			if (right == 0) {
				parser_lexical_error(pctx, "division by zero");
				return left;
			}
			left = bmath_result_t__div(left, right);
			break;
		case '%':
			if (right == 0) {
				parser_lexical_error(pctx, "division by zero");
				return left;
			}
			left = bmath_result_t__mod(left, right);
			break;
		default:
			parser_general_error(
				pctx, "something went wrong parsing factor.\n");
			break;
		}
	}

	return left;
}

static bmath_result_t expr_add(struct parser_context *pctx)
{
	bmath_result_t left, right;
	struct token tok;

	left = expr_factor(pctx);
	while (pctx->lookahead_token.type == TOK_ADDITIVE_OP) {
		tok = pctx->lookahead_token;
		__expect(pctx, tok.type);
		right = expr_factor(pctx);
		switch (tok.tok_attr) {
		case '+':
			left = bmath_result_t__add(left, right);
			break;
		case '-':
			left = bmath_result_t__sub(left, right);
			break;
		default:
			parser_general_error(
				pctx, "something went wrong parsing add.\n");
			break;
		}
	}

	return left;
}

static bmath_result_t expr_shift(struct parser_context *pctx)
{
	bmath_result_t left, right;
	struct token tok;

	left = expr_add(pctx);
	while (pctx->lookahead_token.type == TOK_SHIFT_OP) {
		tok = pctx->lookahead_token;
		__expect(pctx, tok.type);
		right = expr_add(pctx);
		switch (tok.tok_attr) {
		case ATTR_LSHIFT:
			left = bmath_result_t__lshift(left, right);
			break;
		case ATTR_RSHIFT:
			left = bmath_result_t__rshift(left, right);
			break;
		default:
			parser_general_error(
				pctx, "something went wrong parsing shift.\n");
			break;
		}
	}

	return left;
}

static bmath_result_t expr_and(struct parser_context *pctx)
{
	bmath_result_t left;

	left = expr_shift(pctx);
	if (pctx->lookahead_token.tok_attr != '&')
		return left;

	__expect(pctx, TOK_OP);
	return bmath_result_t__and(left, expr_and(pctx));
}

static bmath_result_t expr_xor(struct parser_context *pctx)
{
	bmath_result_t left;

	left = expr_and(pctx);
	if (pctx->lookahead_token.tok_attr != '^')
		return left;

	__expect(pctx, TOK_OP);
	return bmath_result_t__xor(left, expr_xor(pctx));
}

static bmath_result_t expr_or(struct parser_context *pctx)
{
	bmath_result_t left;

	left = expr_xor(pctx);
	if (pctx->lookahead_token.tok_attr != '|')
		return left;

	__expect(pctx, TOK_OP);
	return bmath_result_t__or(left, expr_or(pctx));
}

static bmath_result_t expr_assignment(struct parser_context *pctx)
{
	bmath_result_t ret;
	struct token *token;
	struct symbol *sym = NULL;

	token = &pctx->lookahead_token;
	sym = parser_token_to_symbol(pctx, token);
	ret = expr_or(pctx);
	if (pctx->lookahead_token.type != TOK_ASSIGNMENT) {
		return ret;
	}

	if (!sym || sym->type != SYMBOL_VARIABLE) {
		parser_general_error(pctx, "only variables can be assigned");
		return 0;
	}

	__expect(pctx, TOK_ASSIGNMENT);
	ret = expr(pctx);
	__expect(pctx, TOK_TERMINATOR);

	memcpy(symbol_value(sym), (void *)&ret, sizeof(ret));
	sym->flags |= SYM_FLAG_DEFINED;

	if (pctx->lookahead_token.type == TOK_NULL) {
		return ret;
	}

	return expr(pctx);
}

static bmath_result_t expr(struct parser_context *pctx)
{
	return expr_assignment(pctx);
}
