#pragma once
#include <stddef.h>
#include <stdint.h>

#include "token.h"

struct lexer;
struct lexer_settings {};

void lexer_init(struct lexer *lexer, const char *text, size_t text_len);
struct lexer *lexer_new(const struct lexer_settings *settings);
void lexer_free(struct lexer *lexer);
int lexer_errno(const struct lexer *lexer);
const char *lexer_error_str(const struct lexer *lexer);
struct token lexer_next_token(struct lexer *lexer);
const char *lexer_token_ident(const struct lexer *lexer,
			      const struct token *token);
