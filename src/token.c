#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "token.h"

#define ALPHABET_SIZE (('w' - '_') + 1)

struct token_tbl {
	struct token_tbl *children[ALPHABET_SIZE];
	struct token tok;
	bool keynode;
	bool terminal;
};

static inline int trie_char_to_idx(char c)
{
	return c - '_';
}

static inline bool trie_char_exists(char c)
{
	return trie_char_to_idx(c) >= 0 && trie_char_to_idx(c) < ALPHABET_SIZE;
}

struct token_tbl *token_tbl_new()
{
	struct token_tbl *n;
	n = malloc(sizeof(*n));
	if (!n) {
		return NULL;
	}

	n->keynode = false;
	n->terminal = false;
	n->tok = (struct token){ ATTR_NULL, 0, TOK_NULL };

	memset(n->children, 0, sizeof(n->children));
	return n;
}

void token_tbl_free(struct token_tbl *tbl)
{
	if (!tbl) {
		return;
	}

	for (size_t i = 0; i < sizeof(tbl->children) / sizeof(tbl->children[0]);
	     i++) {
		token_tbl_free(tbl->children[i]);
	}

	free(tbl);
}

struct token *token_tbl_lookup(struct token_tbl *tbl, const char *key)
{
	struct token_tbl *child;

	if (!key[0]) {
		return &tbl->tok;
	}

	if (key[0] && !trie_char_exists(key[0])) {
		return (tbl->terminal) ? &tbl->tok : NULL;
	}

	child = tbl->children[trie_char_to_idx(key[0])];
	if (!child) {
		return NULL;
	}

	return token_tbl_lookup(child, ++key);
}

static int _token_tbl_branch(struct token_tbl *tbl, const char *key,
			     struct token tok)
{
	struct token_tbl *child;
	if (!key[0]) {
		tbl->keynode = true;
		tbl->terminal = true;
		tbl->tok = tok;
		return 0;
	}

	child = token_tbl_new();
	if (!child) {
		return ENOMEM;
	}

	tbl->children[trie_char_to_idx(key[0])] = child;
	return _token_tbl_branch(child, ++key, tok);
}

static int _token_tbl_insert(struct token_tbl *tbl, const char *key,
			     struct token tok)
{
	struct token_tbl *child;
	if (!key[0]) {
		tbl->keynode = true;
		return 0;
	}

	child = tbl->children[trie_char_to_idx(key[0])];
	if (child) {
		return _token_tbl_insert(child, ++key, tok);
	}

	return _token_tbl_branch(tbl, key, tok);
}

int token_tbl_insert(struct token_tbl *tbl, const char *key, struct token tok)
{
	return _token_tbl_insert(tbl, key, tok);
}

int token_tbl_register_func(struct token_tbl *tbl, struct token_func *func)
{
	struct token tok = { .attr = (uint64_t)func->func,
			     .namelen = strlen(func->name),
			     TOK_FUNCTION };
	return token_tbl_insert(tbl, func->name, tok);
}
