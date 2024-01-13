#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>

#include "parser.h"
#include "conversions.h"
#include "shim.h"
#include "util.h"

#define __general_error(fmt, arg...)                    \
do {                                                    \
	fprintf(stderr, "[ERROR]: " fmt, ##arg);        \
	exit(EXIT_FAILURE);                             \
} while(0)

#define __lexical_error(l, fmt, arg...)                                                 \
do {                                                                                    \
	fprintf(stderr, "[PARSE ERROR]: There was an error parsing the expression:\n"); \
	fprintf(stderr, "%s\n", (l)->line);                                             \
	__repeat_character(stderr, (l)->current_column, '~');                           \
	fprintf(stderr, "%c " fmt "\n", '^', ##arg);                                    \
	exit(EXIT_FAILURE);                                                             \
} while (0)

enum token_type {
	TOK_NUMBER = 1,
	TOK_OP,
	TOK_SHIFT_OP,
	TOK_LPAREN,
	TOK_RPAREN,
	TOK_NEGATE,
	TOK_NULL
};

#define ATTR_LPAREN 1
#define ATTR_RPAREN ATTR_LPAREN + 1
#define ATTR_NEGATE ATTR_RPAREN + 1
#define ATTR_LSHIFT ATTR_NEGATE + 1
#define ATTR_RSHIFT ATTR_LSHIFT + 1
#define ATTR_OP_AND ATTR_RSHIFT + 1
#define ATTR_OP_OR ATTR_OP_AND + 1
#define ATTR_OP_XOR ATTR_OP_OR + 1
#define ATTR_NULL UINT64_MAX

struct token {
	uint64_t attr;
	enum token_type type;
};

struct lexer {
	const char *line;
	uint16_t current_column;
	int16_t line_length;
};

// fixable global variable
static struct token lookahead_token;

static bool __is_operator(char character);
static bool __is_x(char character);
static bool __is_start_of_hex(char current_character, char peek);
static bool __is_allowed_character(char character);
static bool __is_illegal_character(char character);

static struct lexer __init_lexer(const char *line, int16_t line_length);
static void __lexer_parse_number(struct lexer *lexer,
				 struct token *token);
static void __lexer_parse_hex(struct lexer *lexer, struct token *token);
static struct token __lexer_get_next_token(struct lexer *lexer);

static void __expect(struct lexer *lexer, enum token_type expected);
static uint64_t __factor(struct lexer *lexer);
static uint64_t __term(struct lexer *lexer);
static uint64_t __expr(struct lexer *lexer);

static uint64_t __perform_parse(struct lexer *lexer)
{
	lookahead_token = __lexer_get_next_token(lexer);
	return __expr(lexer);
}

int16_t parse(const char *infix_expression, uint64_t *out_result)
{
	size_t infix_expression_length;
	struct lexer lexer;
	int16_t out_expression_length = PE_NOTHING_TO_PARSE;

	*out_result = 0;

	if (*infix_expression == '\0')
		return out_expression_length;

	infix_expression_length = strlen(infix_expression);
	if (infix_expression_length > (size_t) INT16_MAX)
		return -PE_EXPRESSION_TOO_LONG;

	lexer = __init_lexer(infix_expression,
		             (int16_t) infix_expression_length);

	*out_result = __perform_parse(&lexer);
	return 1;
}

static bool __is_operator(char character)
{
	return character == '&'
		|| character == '^'
		|| character == '|';
}

static bool __is_allowed_character(char character)
{
	return character == '<'
		|| character == '>'
		|| character == '('
		|| character == ')'
		|| character == '~'
		|| __is_operator(character)
		|| __is_x(character)
		|| isdigit(character)
		|| __ishexnumber(character)
		|| isspace(character);
}

static bool __is_x(char character)
{
	return character == 'x' || character == 'X';
}

static bool __is_start_of_hex(char current_character, char peek)
{
	return current_character == '0' && __is_x(peek);
}


static bool __is_illegal_character(char character)
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

static void __lexer_parse_number(struct lexer *lexer,
                                 struct token *token)
{
	char current_char;

	uint64_t result = 0;
	const char *line_reader = lexer->line + lexer->current_column;

	while ((current_char = *line_reader++) != '\0' && isdigit(current_char)) {
		result = result * 10 + (current_char - '0');
		lexer->current_column++;
	}

	token->type = TOK_NUMBER;
	token->attr = result;
}

static void __lexer_parse_hex(struct lexer *lexer, struct token *token)
{
	const char *line_reader;
	char current_char;
	size_t start_column;
	size_t hex_str_len;
	char *hex_str;

	uint64_t result = 0;

	start_column = lexer->current_column;

	// Skip the 0x to count the remaining characters to extract out
	lexer->current_column += 2;
	line_reader = lexer->line + lexer->current_column;

	while ((current_char = *line_reader++) != '\0' &&
		__ishexnumber(current_char)) {
		lexer->current_column++;
	}

	hex_str_len = lexer->current_column - start_column;

	// We can assume that if there's only 2 characters, then it's just
	// the 0x that parsed.
	if (hex_str_len == 2) {
		__lexical_error(lexer, "No hex code was parsed");
	}

	hex_str = (char *) calloc(hex_str_len + 1, sizeof(char));
	if (hex_str == NULL) {
		__general_error("Unable to allocate enough bytes for hex string parsing.\n");
	}

	strncpy(hex_str, lexer->line + start_column, hex_str_len);

	// This is a redundant check
	if (!str_hex_to_uint64(hex_str, &result)) {
		__lexical_error(lexer, "%s is an invalid hex code.", hex_str);
	}

	free(hex_str);

	token->type = TOK_NUMBER;
	token->attr = result;
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

	while ((current_character = *line_reader++) != '\0')
	{
		peek_character = *line_reader;

		if (isspace(current_character)) {
			lexer->current_column++;
			continue;
		}

		if (current_character == '~') {
			token.type = TOK_NEGATE;
			token.attr = ATTR_NEGATE;
			lexer->current_column++;
			return token;
		}

		if (current_character == '(') {
			token.type = TOK_LPAREN;
			token.attr = ATTR_LPAREN;
			lexer->current_column++;
			return token;
		}

		if (current_character == ')') {
			token.type = TOK_RPAREN;
			token.attr = ATTR_RPAREN;
			lexer->current_column++;
			return token;
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

		if (__is_operator(current_character)) {
			token.type = TOK_OP;
			switch (current_character) {
				case '&': token.attr = ATTR_OP_AND; break;
				case '|': token.attr = ATTR_OP_OR; break;
				case '^': token.attr = ATTR_OP_XOR; break;
				default: break;
			}
			lexer->current_column++;
			return token;
		}

		if (__is_start_of_hex(current_character, peek_character)) {
			__lexer_parse_hex(lexer, &token);
			return token;
		}

		if (isdigit(current_character)) {
			__lexer_parse_number(lexer, &token);
			return token;
		}

		if (__is_illegal_character(current_character)) {
			__lexical_error(lexer, "Illegal character");
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

	__general_error("Expecting token %d, but got %d instead.\n",
	                expected, lookahead_token.type);
}

static uint64_t __factor(struct lexer *lexer)
{
	uint64_t result;
	bool should_negate = false;

	if (lookahead_token.type == TOK_NEGATE) {
		__expect(lexer, TOK_NEGATE);
		should_negate = true;
	}

	if (lookahead_token.type == TOK_LPAREN) {
		__expect(lexer, TOK_LPAREN);
		result = __expr(lexer);
		__expect(lexer, TOK_RPAREN);
		return (should_negate) ? ~result : result;
	}

	result = lookahead_token.attr;
	__expect(lexer, TOK_NUMBER);

	return (should_negate) ? ~result : result;
}

static uint64_t __term(struct lexer *lexer)
{
	uint64_t left, right;
	struct token tok;

	left = __factor(lexer);
	while (true) {
		if (lookahead_token.type != TOK_SHIFT_OP) {
			break;
		}

		tok = lookahead_token;
		__expect(lexer, lookahead_token.type);
		right = __factor(lexer);
		switch (tok.attr) {
			case ATTR_LSHIFT: left <<= right; break;
			case ATTR_RSHIFT: left >>= right; break;
			default: __general_error("Something went wrong parsing term.\n");
		}
	}

	return left;
}

static uint64_t __expr(struct lexer *lexer)
{
	uint64_t left, right;
	struct token tok;

	left = __term(lexer);
	while (true) {
		if (lookahead_token.type != TOK_OP)
			break;

		tok = lookahead_token;
		__expect(lexer, lookahead_token.type);
		right = __term(lexer);
		switch (tok.attr) {
			case ATTR_OP_AND: left &= right; break;
			case ATTR_OP_OR: left |= right; break;
			case ATTR_OP_XOR: left ^= right; break;
			default: __general_error("Something went wrong parsing expr.\n");
		}
	}

	return left;
}
