#pragma once
#include <stdint.h>

#include "symbol.h"
#include "token.h"

struct lexer;
struct lexer_settings {
	struct symbol_tbl *tbl;
	FILE *err_stream;
};

void lexer_init(struct lexer *lexer, const char *line, int16_t line_length);
void lexer_reset(struct lexer *lexer);
struct lexer *lexer_new(const struct lexer_settings *settings);
void lexer_free(struct lexer *lexer);
void lexer_general_error(struct lexer *lexer, char *fmt, ...);
void lexer_lexical_error(struct lexer *lexer, char *fmt, ...);
bool lexer_in_error(struct lexer *lexer);
struct token lexer_next_token(struct lexer *lexer);
