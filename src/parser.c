#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "conversions.h"
#include "parser.h"
#include "util.h"
#include "lookup_tables.h"
#include "token.h"
#include "functions.h"
#include "symbol.h"
#include <sys/types.h>

static struct token *NULL_TOKEN =
	&(struct token){ .type = TOK_NULL, .attr = ATTR_NULL };

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
	return symbol_new(nfunc->name, nfunc->namelen, SYMBOL_FUNCTION, 0,
			  &value, sizeof(value));
}

struct parser_context {
	int max_parse_len;
	bool liberror;
	FILE *err_stream;
	struct symbol_tbl *tbl;
};

#define __general_error(l, fmt, arg...)                           \
	do {                                                      \
		fprintf((l)->err_stream, "[ERROR]: " fmt, ##arg); \
		(l)->ctx->liberror = true;                        \
	} while (0)

#define __lexical_error(l, fmt, arg...)                                                 \
	do {                                                                            \
		fprintf((l)->err_stream,                                                \
			"[PARSE ERROR]: There was an error parsing the expression:\n"); \
		fprintf((l)->err_stream, "%s\n", (l)->line);                            \
		__repeat_character((l)->err_stream, (l)->current_column, '~');          \
		fprintf((l)->err_stream, "%c " fmt "\n", '^', ##arg);                   \
		(l)->ctx->liberror = true;                                              \
	} while (0)

struct lexer {
	const char *line;
	struct parser_context *ctx;
	uint16_t current_column;
	int16_t line_length;
	FILE *err_stream;
	struct token lookahead_token;
};

static inline bool __is_x(char character);
static inline bool __is_start_of_hex(char current_character, char peek);

static struct lexer __init_lexer(struct parser_context *ctx, const char *line,
				 int16_t line_length);
static struct token __lexer_parse_number(struct lexer *lexer);
static struct token __lexer_parse_hex(struct lexer *lexer);
static struct token __lexer_parse_ident(struct lexer *lexer);
static struct token __lexer_get_next_token(struct lexer *lexer);

static void __expect(struct lexer *lexer, enum token_type expected);

static uint64_t expr_number(struct lexer *lexer);
static uint64_t expr_function(struct lexer *lexer);
static uint64_t expr_signed(struct lexer *lexer);
static uint64_t expr_factor(struct lexer *lexer);
static uint64_t expr_add(struct lexer *lexer);
static uint64_t expr_shift(struct lexer *lexer);
static uint64_t expr_and(struct lexer *lexer);
static uint64_t expr_xor(struct lexer *lexer);
static uint64_t expr_or(struct lexer *lexer);
//static uint64_t expr_assignment(struct lexer *lexer);
static uint64_t expr(struct lexer *lexer);

ssize_t str_hex_to_uint64(char *input, ssize_t input_length, uint64_t *result)
{
	ssize_t bytes_parsed = 0;
	const char *input_start = input;

	if (*input++ != '0') {
		errno = EINVAL;
		return -1;
	}

	if (!__is_x(*input++)) {
		errno = EINVAL;
		return -2;
	}

	*result = 0;
	while (__is_allowed_hex(*input)) {
		*result = (*result << 4) + __hex_to_value(*input++);
	}

	bytes_parsed += input - input_start;
	if (bytes_parsed > input_length) {
		errno = E2BIG;
		return -bytes_parsed;
	}

	return bytes_parsed;
}

static uint64_t __perform_parse(struct lexer *lexer)
{
	lexer->lookahead_token = __lexer_get_next_token(lexer);
	return expr(lexer);
}

struct parser_context *parser_new(struct parser_settings *settings)
{
	struct symbol_table_attr tbl_attr;
	int err;
	struct parser_context *ctx = malloc(sizeof(*ctx));
	if (!ctx) {
		return NULL;
	}

	tbl_attr.key_size = 32;

	ctx->tbl = symbol_table_new(&tbl_attr);
	if (!ctx->tbl) {
		goto out_err;
	}

	ctx->liberror = false;
	ctx->max_parse_len = settings->max_parse_len;
	ctx->err_stream = stderr;
	if (settings->err_stream) {
		ctx->err_stream = settings->err_stream;
	}

	for (size_t i = 0;
	     i < sizeof(PREDEFINED_FUNCTIONS) / sizeof(PREDEFINED_FUNCTIONS[0]);
	     i++) {
		struct symbol *sym =
			named_func_to_sym(&PREDEFINED_FUNCTIONS[i]);
		if (!sym) {
			goto out_err;
		}

		err = symbol_table_update(ctx->tbl, sym);
		if (err) {
			goto out_err;
		}
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

	symbol_table_free(ctx->tbl);
	free(ctx);
	return 0;
}

int parse(struct parser_context *ctx, const char *infix_expression, size_t len,
	  uint64_t *out_result)
{
	struct lexer lexer;
	uint64_t result;

	*out_result = 0;

	if (len == 0)
		return PE_NOTHING_TO_PARSE;

	if (len > (size_t)ctx->max_parse_len)
		return PE_EXPRESSION_TOO_LONG;

	lexer = __init_lexer(ctx, infix_expression, (int16_t)len);
	lexer.err_stream = ctx->err_stream;

	result = __perform_parse(&lexer);

	if (ctx->liberror) {
		ctx->liberror = false;
		return PE_PARSE_ERROR;
	}

	*out_result = result;

	return 0;
}

static inline bool __is_x(char character)
{
	switch (character) {
	case 'x':
	case 'X':
		return true;
	default:
		return false;
	}
}

static inline bool __is_start_of_hex(char current_character, char peek)
{
	return current_character == '0' && __is_x(peek);
}

static struct lexer __init_lexer(struct parser_context *ctx, const char *line,
				 int16_t line_length)
{
	struct lexer lexer;

	lexer.line = line;
	lexer.current_column = 0;
	lexer.line_length = line_length;
	lexer.ctx = ctx;

	return lexer;
}

static struct token __lexer_parse_number(struct lexer *lexer)
{
	uint64_t result = 0;
	char *line_reader = (char *)lexer->line + lexer->current_column;
	struct token tok = *NULL_TOKEN;

	while (__is_digit(*line_reader)) {
		result = result * 10 + (*line_reader++ - '0');
	}

	lexer->current_column = line_reader - lexer->line;

	tok.attr = result;
	tok.type = TOK_NUMBER;
	return tok;
}

static struct token __lexer_parse_hex(struct lexer *lexer)
{
	// 8 bytes for 64bit number + 0x
#define MAX_HEX_STR 16 + 2
	uint64_t result = 0;
	char *start = (char *)lexer->line + lexer->current_column;
	struct token tok = *NULL_TOKEN;

	ssize_t bytes_parsed = str_hex_to_uint64(start, MAX_HEX_STR, &result);
	if (bytes_parsed < 0) {
		if (errno == E2BIG) {
			__lexical_error(lexer, "Hex exceeds 8 bytes");
			return tok;
		}

		__lexical_error(lexer, "Invalid hex");
		return tok;
	}

	lexer->current_column += bytes_parsed;

	tok.type = TOK_NUMBER;
	tok.attr = result;
	return tok;
}

static struct token __lexer_parse_ident(struct lexer *lexer)
{
	char *line_reader = (char *)lexer->line + lexer->current_column;
	char *start = line_reader;
	struct symbol *sym = NULL;
	size_t ident_len;
	bool variable = false;
	int err;
	char *ident;
	uint64_t variable_value = 0;

	// account for variable definitions
	if (*line_reader == '$') {
		variable = true;
		line_reader++;
	}

	do {
		if (!__is_allowed_identifier(*line_reader)) {
			//fprintf(stderr, "============= faild character: %c\n",
			//	*line_reader);
			//lexer->current_column += line_reader - start;
			//__lexical_error(
			//	lexer,
			//	"Identifier must only have [_a-zA-Z0-9] characters");
			//return *NULL_TOKEN;
			break;
		}
	} while (!__is_whitespace(*line_reader++));

	ident_len = line_reader - start;
	ident = line_reader - ident_len;
	lexer->current_column += ident_len;

	// just a $
	if ((!ident_len || !(ident_len - 1)) && variable) {
		__lexical_error(lexer, "Identifier is empty");
		return *NULL_TOKEN;
	} else if (!ident_len) {
		return *NULL_TOKEN;
	}

	sym = symbol_table_lookup(lexer->ctx->tbl, ident, ident_len);
	if (sym) {
		return symbol_to_token(sym);
	}

	if (!variable) {
		return *NULL_TOKEN;
	}

	sym = symbol_new(ident, ident_len, SYMBOL_VARIABLE, 0,
			 (void *)&variable_value, sizeof(variable_value));
	if (!sym) {
		__general_error(lexer, "No memory to allocate symbol");
		return *NULL_TOKEN;
	}

	err = symbol_table_update(lexer->ctx->tbl, sym);
	if (err) {
		symbol_free(sym);
		__general_error(lexer,
				"Unable to store identifier into lookup table");
		return *NULL_TOKEN;
	}

	return symbol_to_token(sym);
}

static struct token __lexer_get_next_token(struct lexer *lexer)
{
	char *line_reader = (char *)lexer->line + lexer->current_column;
	struct token token = *NULL_TOKEN;
	char current_character;
	char peek_character;

	// We're already at or past the null character. Perform early return
	// to prevent snooping at memory past the bounds of the array.
	if (lexer->current_column > lexer->line_length - 1) {
		return token;
	}

	while ((current_character = *line_reader++)) {
		peek_character = *line_reader;

		if (__is_digit(current_character)) {
			if (__is_start_of_hex(current_character,
					      peek_character)) {
				return __lexer_parse_hex(lexer);
			}
			return __lexer_parse_number(lexer);
		}

		token.attr = current_character;
		switch (current_character) {
		case '\t':
		case '\n':
		case '\r':
		case ' ':
			lexer->current_column += 1;
			continue;
		case '%':
			token.type = TOK_FACTOR_OP;
			goto out;
		case '/':
			token.type = TOK_FACTOR_OP;
			goto out;
		case '&':
			token.type = TOK_OP;
			goto out;
		case '(':
			token.type = TOK_LPAREN;
			goto out;
		case ')':
			token.type = TOK_RPAREN;
			goto out;
		case '*':
			token.type = TOK_FACTOR_OP;
			goto out;
		case '+':
			token.type = TOK_SIGN;
			goto out;
		case ',':
			token.type = TOK_COMMA;
			goto out;
		case '-':
			token.type = TOK_SIGN;
			goto out;
		case ';':
			token.type = TOK_TERMINATOR;
			goto out;
		case '<':
			if (peek_character == '<') {
				token.type = TOK_SHIFT_OP;
				token.attr = ATTR_LSHIFT;
				lexer->current_column += 1;
				goto out;
			}
			break;
		case '=':
			token.type = TOK_ASSIGNMENT;
			goto out;
		case '>':
			if (peek_character == '>') {
				token.type = TOK_SHIFT_OP;
				token.attr = ATTR_RSHIFT;
				lexer->current_column += 1;
				goto out;
			}
			break;
		case '^':
			token.type = TOK_OP;
			goto out;
		case '|':
			token.type = TOK_OP;
			goto out;
		case '~':
			token.type = TOK_BITWISE_NOT;
			goto out;
		default:
			break;
		}

		token = __lexer_parse_ident(lexer);
		if (token.type != TOK_NULL) {
			return token;
		}

		__lexical_error(lexer, "Illegal character");
		break;
	}

out:
	lexer->current_column += 1;
	return token;
}

static void __expect(struct lexer *lex, enum token_type expected)
{
	if (lex->lookahead_token.type == expected) {
		lex->lookahead_token = __lexer_get_next_token(lex);
		return;
	}

	if (!lex->ctx->liberror) {
		__lexical_error(lex, "Expecting a %s, but got %s instead.",
				token_name(expected),
				token_name(lex->lookahead_token.type));
	}
}

static uint64_t expr_number(struct lexer *lexer)
{
	uint64_t ret;

	if (lexer->lookahead_token.type == TOK_LPAREN) {
		__expect(lexer, TOK_LPAREN);
		ret = expr(lexer);
		__expect(lexer, TOK_RPAREN);
		return ret;
	}

	if (lexer->lookahead_token.type == TOK_IDENT) {
		ret = *(uint64_t *)symbol_value(
			(struct symbol *)lexer->lookahead_token.attr);
		__expect(lexer, TOK_IDENT);
	} else {
		ret = lexer->lookahead_token.attr;
		__expect(lexer, TOK_NUMBER);
	}
	return ret;
}

static uint64_t expr_function(struct lexer *lexer)
{
	int err = 0;
	static uint64_t ops[FUNCTIONS_MAX_OPS] = { 0 };
	uint64_t ret = 0;
	size_t i;
	struct symbol *sym;
	bmath_func_t func;
	struct token tok;

	if (lexer->lookahead_token.type != TOK_IDENT) {
		return expr_number(lexer);
	}

	memset(ops, 0, sizeof(ops));

	tok = lexer->lookahead_token;
	sym = (struct symbol *)tok.attr;
	if (sym->type != SYMBOL_FUNCTION) {
		return expr_number(lexer);
	}

	func = (bmath_func_t) * (uintptr_t *)symbol_value(sym);

	__expect(lexer, TOK_IDENT);
	__expect(lexer, TOK_LPAREN);
	for (i = 0; i < sizeof(ops) / sizeof(ops[0]); i++) {
		if (lexer->lookahead_token.type == TOK_RPAREN) {
			--i;
			break;
		}

		ops[i] = expr(lexer);
		if (lexer->lookahead_token.type != TOK_COMMA) {
			break;
		}
		__expect(lexer, TOK_COMMA);
	}
	__expect(lexer, TOK_RPAREN);

	err = func(&ret, i + 1, ops);
	if (err) {
		__lexical_error(lexer, "%s() returned error code: %d %s",
				symbol_ident(sym), err, str_func_err(err));
		return ret;
	}

	return ret;
}

static uint64_t expr_signed(struct lexer *lexer)
{
#define MAX_STACK 10
	static struct token stack[MAX_STACK] = { 0 };
	struct token tok;

	uint64_t ret;
	int i = -1;
	int in_loop = 0;

	while (1) {
		tok = lexer->lookahead_token;
		switch (tok.type) {
		case TOK_BITWISE_NOT:
		case TOK_SIGN:
			if (!in_loop) {
				in_loop = 1;
				memset(stack, 0, sizeof(stack));
			}
			i++;
			if (i >= MAX_STACK) {
				__lexical_error(
					lexer, "Exceeded max stack depth of %d",
					MAX_STACK);
				return 0;
			}
			stack[i] = tok;
			__expect(lexer, lexer->lookahead_token.type);
			break;
		default:
			ret = expr_function(lexer);
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

static uint64_t expr_factor(struct lexer *lexer)
{
	uint64_t left, right;
	struct token tok;

	left = expr_signed(lexer);
	while (true) {
		if (lexer->lookahead_token.type != TOK_FACTOR_OP) {
			break;
		}

		tok = lexer->lookahead_token;
		__expect(lexer, lexer->lookahead_token.type);
		right = expr_signed(lexer);
		switch (tok.attr) {
		case '*':
			left *= right;
			break;
		case '/':
			if (right == 0) {
				__lexical_error(lexer, "Division by zero");
				return left;
			}
			left /= right;
			break;
		case '%':
			if (right == 0) {
				__lexical_error(lexer, "Division by zero");
				return left;
			}
			left %= right;
			break;
		default:
			__general_error(lexer,
					"Something went wrong parsing term.\n");
		}
	}

	return left;
}

static uint64_t expr_add(struct lexer *lexer)
{
	uint64_t left, right;
	struct token tok;

	left = expr_factor(lexer);
	while (true) {
		if (lexer->lookahead_token.type != TOK_SIGN) {
			break;
		}

		tok = lexer->lookahead_token;
		__expect(lexer, lexer->lookahead_token.type);
		right = expr_factor(lexer);
		switch (tok.attr) {
		case '+':
			left += right;
			break;
		case '-':
			left -= right;
			break;
		default:
			__general_error(lexer,
					"Something went wrong parsing term.\n");
		}
	}

	return left;
}

static uint64_t expr_shift(struct lexer *lexer)
{
	uint64_t left, right;
	struct token tok;

	left = expr_add(lexer);
	while (true) {
		if (lexer->lookahead_token.type != TOK_SHIFT_OP) {
			break;
		}

		tok = lexer->lookahead_token;
		__expect(lexer, lexer->lookahead_token.type);
		right = expr_add(lexer);
		switch (tok.attr) {
		case ATTR_LSHIFT:
			left <<= right;
			break;
		case ATTR_RSHIFT:
			left >>= right;
			break;
		default:
			__general_error(lexer,
					"Something went wrong parsing term.\n");
		}
	}
	return left;
}

static uint64_t expr_and(struct lexer *lexer)
{
	uint64_t left;

	left = expr_shift(lexer);
	if (lexer->lookahead_token.attr != '&')
		return left;

	__expect(lexer, TOK_OP);
	return left & expr_and(lexer);
}

static uint64_t expr_xor(struct lexer *lexer)
{
	uint64_t left;

	left = expr_and(lexer);
	if (lexer->lookahead_token.attr != '^')
		return left;

	__expect(lexer, TOK_OP);
	return left ^ expr_xor(lexer);
}

static uint64_t expr_or(struct lexer *lexer)
{
	uint64_t left;

	left = expr_xor(lexer);
	if (lexer->lookahead_token.attr != '|')
		return left;

	__expect(lexer, TOK_OP);
	return left | expr_or(lexer);
}

static uint64_t expr_assignment(struct lexer *lexer)
{
	uint64_t ret;
	struct token ident;
	struct symbol *sym;
	uint64_t sym_val;

	ident = lexer->lookahead_token;
	ret = expr_or(lexer);

	if (ident.type != TOK_IDENT) {
		return ret;
	}

	if (!ident.attr) {
		__general_error(
			lexer,
			"Token doesn't have a symbol. This should not happen");
		return 0;
	}

	sym = (struct symbol *)ident.attr;
	if (sym->type != SYMBOL_VARIABLE) {
		return ret;
	}

	sym_val = *(uint64_t *)symbol_value(sym);
	if (lexer->lookahead_token.type != TOK_ASSIGNMENT) {
		return ret;
	}

	__expect(lexer, TOK_ASSIGNMENT);
	ret = expr(lexer);
	__expect(lexer, TOK_TERMINATOR);

	memcpy(symbol_value(sym), (void *)&ret, sizeof(ret));

	// this allows multiline assignments
	if (lexer->lookahead_token.type == TOK_NULL) {
		return ret;
	}

	return expr_assignment(lexer);
}

static uint64_t expr(struct lexer *lexer)
{
	return expr_assignment(lexer);
}
