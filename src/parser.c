#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "conversions.h"
#include "parser.h"
#include "util.h"
#include "lookup_tables.h"

struct parser_context {
	int max_parse_len;
	bool liberror;
	FILE *err_stream;
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
};

static const char *lookup_token_name[] = {
	[TOK_NULL] = "null",	     [TOK_NUMBER] = "number",
	[TOK_OP] = "|, ^, or &",     [TOK_SHIFT_OP] = "<<, or >>",
	[TOK_LPAREN] = "(",	     [TOK_RPAREN] = ")",
	[TOK_BITWISE_NOT] = "~",     [TOK_SIGN] = "+, or -",
	[TOK_FACTOR_OP] = "*, or %",
};

static inline const char *token_name(enum token_type tok)
{
	return lookup_token_name[tok];
}

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

struct token {
	uint64_t attr;
	enum token_type type;
};

struct lexer {
	const char *line;
	struct parser_context *ctx;
	uint16_t current_column;
	int16_t line_length;
	FILE *err_stream;
};

// fixable global variable
static struct token lookahead_token;

static inline bool __is_x(char character);
static inline bool __is_start_of_hex(char current_character, char peek);
static inline bool __is_illegal_character(char character);

static struct lexer __init_lexer(struct parser_context *ctx, const char *line,
				 int16_t line_length);
static struct token __lexer_parse_number(struct lexer *lexer);
static struct token __lexer_parse_hex(struct lexer *lexer);
static struct token __lexer_get_next_token(struct lexer *lexer);

static void __expect(struct lexer *lexer, enum token_type expected);

static uint64_t expr_number(struct lexer *lexer);
static uint64_t expr_signed(struct lexer *lexer);
static uint64_t expr_factor(struct lexer *lexer);
static uint64_t expr_add(struct lexer *lexer);
static uint64_t expr_shift(struct lexer *lexer);
static uint64_t expr_and(struct lexer *lexer);
static uint64_t expr_xor(struct lexer *lexer);
static uint64_t expr_or(struct lexer *lexer);
static uint64_t expr(struct lexer *lexer);

size_t str_hex_to_uint64(const char *input, ssize_t input_length,
			 uint64_t *result)
{
	char current_char;
	ssize_t bytes_parsed = 0;

	const char *input_start = input;

	if (*input_start++ != '0') {
		errno = EINVAL;
		return bytes_parsed;
	}

	if (!__is_x(*input_start++)) {
		errno = EINVAL;
		return bytes_parsed;
	}

	bytes_parsed += 2;

	*result = 0;
	do {
		current_char = *input_start++;

		if (!__is_allowed_hex(current_char)) {
			return bytes_parsed;
		}

		*result = (*result << 4) + __hex_to_value(current_char);
	} while (bytes_parsed++ < input_length);

	if (bytes_parsed > input_length) {
		errno = E2BIG;
		return 0;
	}

	return bytes_parsed;
}

static uint64_t __perform_parse(struct lexer *lexer)
{
	lookahead_token = __lexer_get_next_token(lexer);
	return expr(lexer);
}

struct parser_context *parser_new(struct parser_settings *settings)
{
	struct parser_context *ctx = malloc(sizeof(*ctx));
	if (!ctx) {
		return NULL;
	}

	ctx->liberror = false;
	ctx->max_parse_len = settings->max_parse_len;
	ctx->err_stream = stderr;
	if (settings->err_stream) {
		ctx->err_stream = settings->err_stream;
	}

	return ctx;
}

int parser_free(struct parser_context *ctx)
{
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

static inline bool __is_illegal_character(char character)
{
	return !__is_allowed_character(character);
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
	char current_char;
	struct token tok = { .type = TOK_NULL, .attr = ATTR_NULL };

	uint64_t result = 0;
	const char *line_reader = lexer->line + lexer->current_column;

	while ((current_char = *line_reader++) && __is_digit(current_char)) {
		result = result * 10 + (current_char - '0');
		lexer->current_column++;
	}

	tok.type = TOK_NUMBER;
	tok.attr = result;
	return tok;
}

static struct token __lexer_parse_hex(struct lexer *lexer)
{
	// 8 bytes for 64bit number + 0x
#define MAX_HEX_STR 16 + 2
	uint64_t result = 0;
	const char *start = lexer->line + lexer->current_column;
	struct token tok = { .type = TOK_NULL, .attr = ATTR_NULL };

	size_t bytes_parsed = str_hex_to_uint64(start, MAX_HEX_STR, &result);
	if (bytes_parsed == 0) {
		if (errno == EOVERFLOW) {
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

static struct token __lexer_get_next_token(struct lexer *lexer)
{
	const char *line_reader = lexer->line + lexer->current_column;
	struct token token;
	char current_character;
	char peek_character;

	token.type = TOK_NULL;
	token.attr = ATTR_NULL;

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
			} else {
				return __lexer_parse_number(lexer);
			}
		}

		switch (current_character) {
		case ' ':
		case '\n':
		case '\t':
		case '\r':
			lexer->current_column++;
			continue;
		case '~':
			token.type = TOK_BITWISE_NOT;
			token.attr = ATTR_BITWISE_NOT;
			lexer->current_column++;
			return token;
		case '(':
			token.type = TOK_LPAREN;
			token.attr = ATTR_LPAREN;
			lexer->current_column++;
			return token;
		case ')':
			token.type = TOK_RPAREN;
			token.attr = ATTR_RPAREN;
			lexer->current_column++;
			return token;
		case '&':
			token.type = TOK_OP;
			token.attr = ATTR_OP_AND;
			lexer->current_column++;
			return token;
		case '|':
			token.type = TOK_OP;
			token.attr = ATTR_OP_OR;
			lexer->current_column++;
			return token;
		case '^':
			token.type = TOK_OP;
			token.attr = ATTR_OP_XOR;
			lexer->current_column++;
			return token;
		case '+':
			token.type = TOK_SIGN;
			token.attr = ATTR_SIGN_PLUS;
			lexer->current_column++;
			return token;
		case '-':
			token.type = TOK_SIGN;
			token.attr = ATTR_SIGN_MINUS;
			lexer->current_column++;
			return token;
		case '*':
			token.type = TOK_FACTOR_OP;
			token.attr = ATTR_FACTOR_OP_MUL;
			lexer->current_column++;
			return token;
		case '%':
			token.type = TOK_FACTOR_OP;
			token.attr = ATTR_FACTOR_OP_MOD;
			lexer->current_column++;
			return token;
		default:
			break;
		}

		if (current_character == '<' && peek_character == '<') {
			token.type = TOK_SHIFT_OP;
			token.attr = ATTR_LSHIFT;
			lexer->current_column += 2;
			return token;
		}

		if (current_character == '>' && peek_character == '>') {
			token.type = TOK_SHIFT_OP;
			token.attr = ATTR_RSHIFT;
			lexer->current_column += 2;
			return token;
		}

		if (unlikely(__is_illegal_character(current_character))) {
			__lexical_error(lexer, "Illegal character");
			return token;
		}

		lexer->current_column++;
	}

	return token;
}

static void __expect(struct lexer *lex, enum token_type expected)
{
	if (lookahead_token.type == expected) {
		lookahead_token = __lexer_get_next_token(lex);
		return;
	}

	if (!lex->ctx->liberror) {
		__lexical_error(lex, "Expecting a %s, but got %s instead.",
				token_name(expected),
				token_name(lookahead_token.type));
	}
}

static uint64_t expr_number(struct lexer *lexer)
{
	uint64_t ret;

	if (lookahead_token.type == TOK_LPAREN) {
		__expect(lexer, TOK_LPAREN);
		ret = expr(lexer);
		__expect(lexer, TOK_RPAREN);
		return ret;
	}

	ret = lookahead_token.attr;
	__expect(lexer, TOK_NUMBER);
	return ret;
}

static uint64_t expr_signed(struct lexer *lexer)
{
#define MAX_STACK 10
	struct token stack[MAX_STACK] = { 0 };
	struct token tok;

	uint64_t ret;
	int i = -1;
	bool exit = false;

	while (!exit) {
		tok = lookahead_token;
		switch (tok.type) {
		case TOK_BITWISE_NOT:
		case TOK_SIGN:
			i++;
			if (i >= MAX_STACK) {
				__lexical_error(
					lexer, "Exceeded max stack depth of %d",
					MAX_STACK);
				return 0;
			}
			stack[i] = tok;
			__expect(lexer, lookahead_token.type);
			break;
		default:
			exit = true;
			ret = expr_number(lexer);
		}
	}

	for (int j = i; j >= 0; j--) {
		switch (stack[j].type) {
		case TOK_BITWISE_NOT:
			ret = ~ret;
		case TOK_SIGN:
			if (stack[j].attr == ATTR_SIGN_MINUS) {
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
		if (lookahead_token.type != TOK_FACTOR_OP) {
			break;
		}

		tok = lookahead_token;
		__expect(lexer, lookahead_token.type);
		right = expr_signed(lexer);
		switch (tok.attr) {
		case ATTR_FACTOR_OP_MUL:
			left *= right;
			break;
		case ATTR_FACTOR_OP_MOD:
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
		if (lookahead_token.type != TOK_SIGN) {
			break;
		}

		tok = lookahead_token;
		__expect(lexer, lookahead_token.type);
		right = expr_signed(lexer);
		switch (tok.attr) {
		case ATTR_SIGN_PLUS:
			left += right;
			break;
		case ATTR_SIGN_MINUS:
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
		if (lookahead_token.type != TOK_SHIFT_OP) {
			break;
		}

		tok = lookahead_token;
		__expect(lexer, lookahead_token.type);
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
	if (lookahead_token.attr != ATTR_OP_AND)
		return left;

	__expect(lexer, TOK_OP);
	return left & expr_and(lexer);
}

static uint64_t expr_xor(struct lexer *lexer)
{
	uint64_t left;

	left = expr_and(lexer);
	if (lookahead_token.attr != ATTR_OP_XOR)
		return left;

	__expect(lexer, TOK_OP);
	return left ^ expr_xor(lexer);
}

static uint64_t expr_or(struct lexer *lexer)
{
	uint64_t left;

	left = expr_xor(lexer);
	if (lookahead_token.attr != ATTR_OP_OR)
		return left;

	__expect(lexer, TOK_OP);
	return left | expr_or(lexer);
}

static uint64_t expr(struct lexer *lexer)
{
	return expr_or(lexer);
}
