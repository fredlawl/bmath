#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>

#include "conversions.h"
#include "parser.h"
#include "util.h"
#include "lookup_tables.h"

static bool liberror = false;

#define __general_error(fmt, arg...)                     \
	do {                                             \
		fprintf(stderr, "[ERROR]: " fmt, ##arg); \
		liberror = true;                         \
	} while (0)

#define __lexical_error(l, fmt, arg...)                                                 \
	do {                                                                            \
		fprintf(stderr,                                                         \
			"[PARSE ERROR]: There was an error parsing the expression:\n"); \
		fprintf(stderr, "%s\n", (l)->line);                                     \
		__repeat_character(stderr, (l)->current_column, '~');                   \
		fprintf(stderr, "%c " fmt "\n", '^', ##arg);                            \
		liberror = true;                                                        \
	} while (0)

enum token_type {
	TOK_NUMBER = 256,
	TOK_AND,
	TOK_OR,
	TOK_XOR,
	TOK_SHIFT_LEFT,
	TOK_SHIFT_RIGHT,
	TOK_NEGATE,
};
#define TOK_IDX(tok) (tok - 256)
static const char *lookup_token_name[] = {
	[TOK_IDX(TOK_NUMBER)] = "number", [TOK_IDX(TOK_AND)] = "&",
	[TOK_IDX(TOK_OR)] = "|",	  [TOK_IDX(TOK_XOR)] = "^",
	[TOK_IDX(TOK_SHIFT_LEFT)] = "<<", [TOK_IDX(TOK_SHIFT_RIGHT)] = ">>",
	[TOK_IDX(TOK_NEGATE)] = "~",
};

static char tok_name_charbuf[1] = { 0 };
static inline const char *token_name(int tag)
{
	if (tag < 256) {
		tok_name_charbuf[0] = (char)tag;
		return tok_name_charbuf;
	}
	return lookup_token_name[TOK_IDX(tag)];
}

struct token {
	int tag;
	union {
		uint64_t i;
	};
};

struct lexer {
	const char *line;
	uint16_t current_column;
	int16_t line_length;
	struct token next_token;
	struct token current_token;
};

static inline bool __is_x(char character);
static inline bool __is_start_of_hex(char current_character, char peek);
static inline bool __is_illegal_character(char character);

static struct lexer __init_lexer(const char *line, int16_t line_length);
static struct token __lexer_parse_number(struct lexer *lexer);
static struct token __lexer_parse_hex(struct lexer *lexer);
static inline struct token __lexer_get_next_token(struct lexer *lexer);

static inline void __expect(struct lexer *lexer, int expected_tag);
//static inline uint64_t __factor(struct lexer *lexer);
//static inline uint64_t __term(struct lexer *lexer);
static inline uint64_t __expr(struct lexer *lexer);

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
	// prime the next token
	__lexer_get_next_token(lexer);
	return __expr(lexer);
}

int parse(const char *infix_expression, size_t len, uint64_t *out_result)
{
	struct lexer lexer;
	uint64_t result;

	*out_result = 0;

	if (len == 0)
		return PE_NOTHING_TO_PARSE;

	if (len > (size_t)P_MAX_EXP_LEN)
		return PE_EXPRESSION_TOO_LONG;

	lexer = __init_lexer(infix_expression, (int16_t)len);

	result = __perform_parse(&lexer);

	if (liberror) {
		liberror = false;
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

static struct lexer __init_lexer(const char *line, int16_t line_length)
{
	struct lexer lexer;

	lexer.line = line;
	lexer.current_column = 0;
	lexer.line_length = line_length;

	return lexer;
}

static struct token __lexer_parse_number(struct lexer *lexer)
{
	char current_char;
	struct token tok;

	uint64_t result = 0;
	const char *line_reader = lexer->line + lexer->current_column;

	while ((current_char = *line_reader++) && __is_digit(current_char)) {
		result = result * 10 + (current_char - '0');
		lexer->current_column++;
	}

	tok.tag = TOK_NUMBER;
	tok.i = result;
	return tok;
}

static struct token __lexer_parse_hex(struct lexer *lexer)
{
	// 8 bytes for 64bit number + 0x
#define MAX_HEX_STR 16 + 2
	uint64_t result = 0;
	const char *start = lexer->line + lexer->current_column;
	struct token tok = { .tag = 0, .i = 0 };

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

	tok.tag = TOK_NUMBER;
	tok.i = result;
	return tok;
}

static struct token __next_token(struct lexer *lexer)
{
	const char *line_reader = lexer->line + lexer->current_column;
	struct token token = { 0 };
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
			lexer->current_column++;
			token.tag = TOK_NEGATE;
			return token;
		case '&':
			lexer->current_column++;
			token.tag = TOK_AND;
			return token;
		case '|':
			lexer->current_column++;
			token.tag = TOK_OR;
			return token;
		case '^':
			lexer->current_column++;
			token.tag = TOK_XOR;
			return token;
		case '(':
		case ')':
			lexer->current_column++;
			token.tag = current_character;
			return token;
		default:
			break;
		}

		if (current_character == '<' && peek_character == '<') {
			token.tag = TOK_SHIFT_LEFT;
			lexer->current_column += 2;
			return token;
		}

		if (current_character == '>' && peek_character == '>') {
			token.tag = TOK_SHIFT_RIGHT;
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

static inline struct token __lexer_get_next_token(struct lexer *lexer)
{
	lexer->current_token = lexer->next_token;
	lexer->next_token = __next_token(lexer);
	return lexer->current_token;
}

static inline void __expect(struct lexer *lex, int expected_tag)
{
	__lexer_get_next_token(lex);
	/*if (lex->next_token.tag == expected_tag) {
		__lexer_get_next_token(lex);
		return;
	}

	if (!liberror) {
		__lexical_error(lex, "Expecting a %s, but got %s instead.",
				token_name(expected_tag),
				token_name(lex->next_token.tag));
	}*/
}

static inline uint64_t __expr_t(struct lexer *lexer, uint64_t running);

static inline uint64_t __factor(struct lexer *lexer, uint64_t running)
{
	uint64_t result;
	struct token tok = __lexer_get_next_token(lexer);

	if (tok.tag == TOK_NEGATE) {
		//__expect(lexer, '~');
		return ~__factor(lexer, running);
	}

	if (tok.tag == '(') {
		__expect(lexer, '(');
		result = __expr_t(lexer, lexer->current_token.i);
		__expect(lexer, ')');
		return result;
	}

	return tok.i;
}

/*static inline uint64_t __factor(struct lexer *lexer)
{
	uint64_t result;
	struct token tok = __lexer_get_next_token(lexer);

	if (tok.tag == TOK_NEGATE) {
		return ~__factor(lexer);
	}

	if (tok.tag == '(') {
		result = __expr(lexer);
		__expect(lexer, ')');
		return result;
	}

	return tok.i;
}*/

static inline uint64_t __term(struct lexer *lexer, uint64_t running)
{
	uint64_t left = __factor(lexer, running);
	switch (lexer->next_token.tag) {
	case TOK_SHIFT_LEFT:
		__expect(lexer, TOK_SHIFT_LEFT);
		return __term(lexer, left << running);
	case TOK_SHIFT_RIGHT:
		__expect(lexer, TOK_SHIFT_RIGHT);
		return __term(lexer, left >> running);
	}

	return left;
}

/*static inline uint64_t __term(struct lexer *lexer)
{
	uint64_t left = __factor(lexer);
	switch (lexer->next_token.tag) {
	case TOK_SHIFT_LEFT:
		__expect(lexer, TOK_SHIFT_LEFT);
		return left << __term(lexer);
	case TOK_SHIFT_RIGHT:
		__expect(lexer, TOK_SHIFT_RIGHT);
		return left >> __term(lexer);
	}

	return left;
}*/

static inline uint64_t __expr_t(struct lexer *lexer, uint64_t running)
{
	uint64_t left = __term(lexer, running);
	switch (lexer->next_token.tag) {
	case TOK_AND:
		__expect(lexer, TOK_AND);
		return __term(lexer, left & running);
	case TOK_OR:
		__expect(lexer, TOK_OR);
		return __term(lexer, left | running);
	case TOK_XOR:
		__expect(lexer, TOK_XOR);
		return __term(lexer, left ^ running);
	}

	return left;
}

/*static inline uint64_t __expr(struct lexer *lexer)
{
	uint64_t left = __term(lexer);
	switch (lexer->next_token.tag) {
	case TOK_AND:
		__expect(lexer, TOK_AND);
		return left & __expr(lexer);
	case TOK_OR:
		__expect(lexer, TOK_OR);
		return left | __expr(lexer);
	case TOK_XOR:
		__expect(lexer, TOK_XOR);
		return left ^ __expr(lexer);
	}

	return left;
}*/

static inline uint64_t __expr(struct lexer *lexer)
{
	return __expr_t(lexer, 0);
}
