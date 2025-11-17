#include "type.h"
#include <asm-generic/errno-base.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "conversions.h"
#include "lexer.h"
#include "lookup_tables.h"
#include "token.h"

#define LEXER_ERR_STR_BYTES 128
struct lexer {
	const char *text;
	size_t text_len;
	size_t current_column;
	size_t current_line;
	int liberror;
	char error_message[LEXER_ERR_STR_BYTES];
	char *last_identifier;
};

static struct token *NULL_TOKEN = &(struct token){ .type = TOK_NULL,
						   .tok_attr = ATTR_NULL,
						   .offset = 0,
						   .len = 0 };

/*
 * Returns null-terminated string of the tokens identifier.
 * Tokens that are not identifiers will return null.
 * This should be copied by the caller immediately coming
 * across an identifier token, as the pointer may not last
 * by next call to lexer_next_token()
 * In the future this could change to be persisted
 * until the next lexer_init() call
 */
const char *lexer_token_ident(const struct lexer *lexer,
			      const struct token *token)
{
	if (token->type != TOK_IDENT && token->type != TOK_VARIABLE) {
		return NULL;
	}

	return lexer->last_identifier;
}

static void lexer_token_ident_free(struct lexer *lexer)
{
	if (lexer->last_identifier)
		free(lexer->last_identifier);
	lexer->last_identifier = NULL;
}

static void lexer_lexical_error(struct lexer *lexer, char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vsnprintf(lexer->error_message, LEXER_ERR_STR_BYTES, fmt, args);
	va_end(args);

	lexer->liberror = EINVAL;
}

int lexer_errno(const struct lexer *lexer)
{
	return lexer->liberror;
}

const char *lexer_error_str(const struct lexer *lexer)
{
	return lexer->error_message;
}

static inline bool __is_x(char character);

static struct token __lexer_parse_number(struct lexer *lexer);
static struct token __lexer_parse_hex(struct lexer *lexer);
static struct token __lexer_parse_octal(struct lexer *lexer);
static struct token __lexer_parse_ident(struct lexer *lexer);

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

static inline int __is_allowed_octal(char input)
{
	switch (input) {
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
		return true;
	default:
		return false;
	}
}

ssize_t str_octal_to_utin64(char *input, ssize_t input_length, uint64_t *result)
{
	ssize_t bytes_parsed = 0;
	const char *input_start = input;

	if (*input++ != '0') {
		errno = EINVAL;
		return -1;
	}

	*result = 0;
	while (__is_allowed_octal(*input)) {
		*result = (*result << 3) + (*input++ - '0');
	}

	bytes_parsed += input - input_start;
	if (bytes_parsed > input_length) {
		errno = E2BIG;
		return -bytes_parsed;
	}

	return bytes_parsed;
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

static void lexer_reset(struct lexer *lexer)
{
	lexer->text = NULL;
	lexer->current_column = 0;
	lexer->text_len = 0;
	lexer->liberror = 0;
	lexer->current_line = 0;
	lexer_token_ident_free(lexer);
}

void lexer_init(struct lexer *lexer, const char *text, size_t text_len)
{
	lexer_reset(lexer);
	lexer->text = text;
	lexer->text_len = text_len;
}

struct lexer *lexer_new(const struct lexer_settings *settings)
{
	struct lexer *lexer = NULL;

	lexer = calloc(1, sizeof(*lexer));
	if (!lexer) {
		return lexer;
	}

	return lexer;
}

void lexer_free(struct lexer *lexer)
{
	if (!lexer)
		return;

	free(lexer);
}

static struct token __lexer_parse_number(struct lexer *lexer)
{
	uint64_t result = 0;
	char *line_reader = (char *)lexer->text + lexer->current_column;
	struct token tok = { .type = TOK_NUMBER,
			     .offset = lexer->current_column,
			     .line = lexer->current_line };

	// TODO: Should have a E2BIG?
	while (__is_digit(*line_reader)) {
		result = result * 10 + (*line_reader++ - '0');
	}

	lexer->current_column = line_reader - lexer->text;

	tok.tok_ret = bmath_result_t__from_uint64(result);
	tok.len = lexer->current_column - tok.offset;
	return tok;
}

static struct token __lexer_parse_hex(struct lexer *lexer)
{
	// 8 bytes for 64bit number + 0x
#define MAX_HEX_STR 16 + 2
	uint64_t result = 0;
	char *start = (char *)lexer->text + lexer->current_column;
	struct token tok = { .type = TOK_NUMBER,
			     .line = lexer->current_line,
			     .offset = lexer->current_column };

	ssize_t bytes_parsed = str_hex_to_uint64(start, MAX_HEX_STR, &result);
	if (bytes_parsed < 0) {
		lexer->current_column += -bytes_parsed;
		tok.len = -bytes_parsed;
		if (errno == E2BIG) {
			lexer_lexical_error(lexer, "hex exceeds 8 bytes");
			return tok;
		}
	}

	tok.len = bytes_parsed;
	lexer->current_column += bytes_parsed;

	// just the 0x
	if (bytes_parsed <= 2) {
		lexer_lexical_error(lexer, "invalid hex");
		return tok;
	}

	tok.tok_ret = bmath_result_t__from_uint64(result);
	return tok;
}

// The weird thing about octal is that if just 1 digit is > 7, then we're actually parsing a number.
// Therefore, this either should return a number or error on invalid octal.
static struct token __lexer_parse_octal(struct lexer *lexer)
{
	// (64 / 3) + (64 % 3) = 22
	// 22 + 1 for the leading 0
#define MAX_OCTAL_STR 22 + 1
	ssize_t bytes_parsed;
	uint64_t result = 0;
	char *start = (char *)lexer->text + lexer->current_column;
	struct token tok = { .type = TOK_NUMBER,
			     .offset = lexer->current_column,
			     .line = lexer->current_line };

	bytes_parsed = str_octal_to_utin64(start, MAX_OCTAL_STR, &result);
	if (bytes_parsed < 0) {
		lexer->current_column += -bytes_parsed;
		tok.len = -bytes_parsed;
		if (errno == E2BIG) {
			lexer_lexical_error(lexer, "octal exceeds 12 bytes");
			return tok;
		}
	}

	lexer->current_column += bytes_parsed;
	tok.len = bytes_parsed;

	// just the leading 0
	if (bytes_parsed == 1) {
		lexer_lexical_error(lexer, "invalid octal");
		return tok;
	}

	tok.tok_ret = bmath_result_t__from_uint64(result);
	return tok;
}

static struct token __lexer_parse_ident(struct lexer *lexer)
{
	char *line_reader = (char *)lexer->text + lexer->current_column;
	char *start = line_reader;
	size_t ident_len;
	struct token tok = { .type = TOK_IDENT,
			     .line = lexer->current_line,
			     .offset = lexer->current_column };

	if (*line_reader == '@') {
		line_reader++;
	}

	// just the @ was reported
	if (!*line_reader || __is_whitespace(*line_reader)) {
		lexer_token_ident_free(lexer);
		lexer->current_column += 1;
		tok.len = 1;
		lexer_lexical_error(lexer, "missing identifer");
		return tok;
	}

	while (__is_allowed_identifier(*++line_reader))
		;

	ident_len = line_reader - start;
	lexer->current_column += ident_len;
	tok.len = ident_len;

	lexer_token_ident_free(lexer);
	lexer->last_identifier =
		calloc(ident_len + 1, sizeof(*lexer->last_identifier));
	if (!lexer->last_identifier) {
		lexer_lexical_error(lexer, "identifier unallocated");
		lexer->liberror = ENOMEM;
		return tok;
	}
	strncpy(lexer->last_identifier, (char *)lexer->text + tok.offset,
		ident_len);

	return tok;
}

static size_t lexer_eat_line(struct lexer *lexer)
{
	char *line_reader = (char *)lexer->text + lexer->current_column;
	char *start = line_reader;

	while (*line_reader && *++line_reader != '\n')
		;

	lexer->current_column += line_reader - start;
	return line_reader - start;
}

struct token lexer_next_token(struct lexer *lexer)
{
	char *line_reader = (char *)lexer->text + lexer->current_column;
	struct token token = *NULL_TOKEN;
	char current_character;
	char peek_character;
	lexer->liberror = 0;

	// We're already at or past the null character. Perform early return
	// to prevent snooping at memory past the bounds of the array.
	if (lexer->current_column > lexer->text_len - 1) {
		lexer->liberror = EOF;
		token.offset = lexer->text_len;
		token.line = lexer->current_line;
		return token;
	}

	while ((current_character = *line_reader++)) {
		peek_character = *line_reader;

		if (__is_digit(current_character)) {
			switch (current_character) {
			case '0':
				switch (peek_character) {
				case 'x':
				case 'X':
					return __lexer_parse_hex(lexer);
				default:
					if (__is_digit(peek_character)) {
						return __lexer_parse_octal(
							lexer);
					}
					return __lexer_parse_number(lexer);
				}
			default:
				return __lexer_parse_number(lexer);
			}
		}

		token.tok_attr = current_character;
		token.offset = lexer->current_column;
		token.len = 1;
		token.line = lexer->current_line;
		lexer->current_column += 1;
		switch (current_character) {
		case '\t':
		case '\n':
		case '\r':
		case ' ':
			if (lexer->current_column > lexer->text_len - 1) {
				lexer->liberror = EOF;
				token.type = TOK_NULL;
				token.len = 0;
				return token;
			}

			if (current_character == '\n') {
				lexer->text_len -= lexer->current_column;
				lexer->current_line++;
				lexer->text =
					lexer->text + lexer->current_column;
				lexer->current_column = 0;
			}

			continue;
		case '%':
			token.type = TOK_FACTOR_OP;
			goto out;
		case '/':
			if (peek_character == '/') {
				token.type = TOK_COMMENT;
				lexer->current_column += 1;
				token.len += 1 + lexer_eat_line(lexer);
				goto out;
			}
			token.type = TOK_FACTOR_OP;
			goto out;
		case '#':
			token.type = TOK_COMMENT;
			token.len += lexer_eat_line(lexer);
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
			token.type = TOK_ADDITIVE_OP;
			goto out;
		case ',':
			token.type = TOK_COMMA;
			goto out;
		case '-':
			token.type = TOK_ADDITIVE_OP;
			goto out;
		case ';':
			token.type = TOK_TERMINATOR;
			goto out;
		case '<':
			if (peek_character == '<') {
				token.type = TOK_SHIFT_OP;
				token.tok_attr = ATTR_LSHIFT;
				lexer->current_column += 1;
				token.len += 1;
				goto out;
			}
			break;
		case '=':
			token.type = TOK_ASSIGNMENT;
			goto out;
		case '>':
			if (peek_character == '>') {
				token.type = TOK_SHIFT_OP;
				token.tok_attr = ATTR_RSHIFT;
				lexer->current_column += 1;
				token.len += 1;
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
		case '@':
			lexer->current_column -= 1;
			token = __lexer_parse_ident(lexer);
			token.type = TOK_VARIABLE;
			goto out;
		default:
			break;
		}

		lexer->current_column -= 1;
		token = __lexer_parse_ident(lexer);
		break;
	}

out:
	return token;
}
