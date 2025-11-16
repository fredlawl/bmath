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
#include "functions.h"
#include "lexer.h"
#include "lookup_tables.h"
#include "token.h"
#include "symbol.h"
#include "util.h"

struct lexer {
	const char *line;
	struct symbol_tbl *tbl;
	uint16_t current_column;
	int16_t line_length;
	FILE *err_stream;
	bool liberror;
};

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

void lexer_general_error(struct lexer *lexer, char *fmt, ...)
{
	va_list args;

	fprintf(lexer->err_stream, "[ERROR]: ");

	va_start(args, fmt);
	vfprintf(lexer->err_stream, fmt, args);
	va_end(args);
	lexer->liberror = true;
}

void lexer_lexical_error(struct lexer *lexer, char *fmt, ...)
{
	va_list args;
	if (lexer_in_error(lexer))
		return;

	fprintf(lexer->err_stream,
		"[PARSE ERROR]: There was an error parsing the expression:\n");
	fprintf(lexer->err_stream, "%s\n", lexer->line);
	__repeat_character(lexer->err_stream, lexer->current_column, '~');
	fprintf(lexer->err_stream, "^ ");
	va_start(args, fmt);
	vfprintf(lexer->err_stream, fmt, args);
	va_end(args);
	putc('\n', lexer->err_stream);
	lexer->liberror = true;
}

bool lexer_in_error(struct lexer *lexer)
{
	return lexer->liberror;
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

void lexer_init(struct lexer *lexer, const char *line, int16_t line_length)
{
	lexer_reset(lexer);
	lexer->line = line;
	lexer->current_column = 0;
	lexer->line_length = line_length;
}

void lexer_reset(struct lexer *lexer)
{
	lexer->line = NULL;
	lexer->current_column = 0;
	lexer->line_length = 0;
	lexer->liberror = false;
}

struct lexer *lexer_new(const struct lexer_settings *settings)
{
	int err;
	struct lexer *lexer = NULL;

	lexer = calloc(1, sizeof(*lexer));
	if (!lexer) {
		return lexer;
	}

	lexer->tbl = settings->tbl;
	lexer->err_stream = settings->err_stream;
	lexer_reset(lexer);

	// The only reason why this is here is so that callers don't need to preload the table themselves...
	// I also don't think the parser is responsible for setting up the table either, and it currently does, but both parser and lexer depend on the symbol table.
	//
	// TODO: Something to think about:
	// Should lexer_reset() purge & reset symbol table:
	//  On one hand, no because handling whole files will need to be thought about since they're parsed per line
	//  One the other, a function should be added to clean symbol table state, and lexer needs to reset to defaults after that state is cleaned
	for (size_t i = 0;
	     i < sizeof(PREDEFINED_FUNCTIONS) / sizeof(PREDEFINED_FUNCTIONS[0]);
	     i++) {
		struct symbol *sym =
			named_func_to_sym(&PREDEFINED_FUNCTIONS[i]);

		if (!sym) {
			continue;
		}

		err = symbol_table_update(settings->tbl, sym);
		if (err) {
			symbol_free(sym);
		}
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
			lexer_lexical_error(lexer, "Hex exceeds 8 bytes");
			return tok;
		}

		lexer_lexical_error(lexer, "Invalid hex");
		return tok;
	}

	lexer->current_column += bytes_parsed;

	tok.type = TOK_NUMBER;
	tok.attr = result;
	return tok;
}

// The weird thing about octal is that if just 1 digit is > 7, then we're acutally parsing a number.
// Therefore, this either should return a number or error on invalid octal.
static struct token __lexer_parse_octal(struct lexer *lexer)
{
	// (64 / 3) + (64 % 3) = 22
	// 22 + 1 for the leading 0
#define MAX_OCTAL_STR 22 + 1
	uint64_t result = 0;
	char *start = (char *)lexer->line + lexer->current_column;
	struct token tok = *NULL_TOKEN;

	ssize_t bytes_parsed =
		str_octal_to_utin64(start, MAX_OCTAL_STR, &result);
	if (bytes_parsed < 0) {
		if (errno == E2BIG) {
			lexer_lexical_error(lexer, "Octal exceeds 12 bytes");
			return tok;
		}

		lexer_lexical_error(lexer, "Invalid octal");
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
	if (*line_reader == '@') {
		variable = true;
		line_reader++;
	}

	while (__is_allowed_identifier(*line_reader++))
		;

	// lop off last character
	line_reader--;

	ident_len = line_reader - start;
	ident = line_reader - ident_len;
	lexer->current_column += ident_len;

	// just a $
	if ((!ident_len || !(ident_len - 1)) && variable) {
		lexer_lexical_error(lexer, "Identifier is empty");
		return *NULL_TOKEN;
	} else if (!ident_len) {
		return *NULL_TOKEN;
	} else if (ident_len > 32) {
		lexer_lexical_error(
			lexer, "Identifer too long. Max %d characters got %d",
			32, (int)ident_len);
		return *NULL_TOKEN;
	}

	sym = symbol_table_lookup(lexer->tbl, ident, ident_len);
	// lookup can be greedy, so ensure that we match exactly
	// TODO: Fixup lookup to avoid this final comparison
	if (sym && sym->ident_len == ident_len &&
	    !strncmp(symbol_ident(sym), ident, ident_len)) {
		return symbol_to_token(sym);
	}

	if (!variable) {
		return *NULL_TOKEN;
	}

	sym = symbol_new(ident, ident_len, SYMBOL_VARIABLE, 0,
			 (void *)&variable_value, sizeof(variable_value));
	if (!sym) {
		lexer_lexical_error(lexer, "No memory to allocate symbol");
		return *NULL_TOKEN;
	}

	err = symbol_table_update(lexer->tbl, sym);
	if (err) {
		symbol_free(sym);
		lexer_lexical_error(
			lexer, "Unable to store identifier into lookup table");
		return *NULL_TOKEN;
	}

	return symbol_to_token(sym);
}

struct token lexer_next_token(struct lexer *lexer)
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
			switch (current_character) {
			case '0':
				switch (peek_character) {
				case 'x':
				case 'X':
					return __lexer_parse_hex(lexer);
				default:
					return __lexer_parse_octal(lexer);
				}
			default:
				return __lexer_parse_number(lexer);
			}
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

		lexer_lexical_error(lexer, "Illegal character");
		break;
	}

out:
	lexer->current_column += 1;
	return token;
}
