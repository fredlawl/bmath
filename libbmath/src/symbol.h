#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "token.h"

enum symbol_type { SYMBOL_NONE = 0, SYMBOL_FUNCTION, SYMBOL_VARIABLE };

struct symbol {
	uint32_t flags;
	enum symbol_type type;
	size_t ident_len;
	uint8_t data[];
};

struct symbol_table_attr {
	size_t key_size;
};

static inline struct symbol *symbol_new(const char *name, size_t name_len,
					enum symbol_type type, int flags,
					void *value, size_t value_len)
{
	struct symbol *sym = NULL;
	sym = calloc(1, sizeof(*sym) + name_len + 1 + value_len);
	if (!sym) {
		return NULL;
	}
	memcpy(sym->data, name, name_len);
	// the name_len + 1 allows for a null byte inbetween to print the identifier later
	memcpy(sym->data + name_len + 1, value, value_len);
	sym->flags = flags;
	sym->type = type;
	sym->ident_len = name_len;
	return sym;
}

static inline void *symbol_value(struct symbol *sym)
{
	return sym->data + sym->ident_len + 1;
}

static inline const char *symbol_ident(struct symbol *sym)
{
	return (const char *)sym->data;
}

static inline void symbol_free(struct symbol *sym)
{
	if (!sym) {
		return;
	}

	free(sym);
	sym = NULL;
}

static inline struct token symbol_to_token(struct symbol *sym)
{
	return (struct token){ .attr = (uint64_t)sym, .type = TOK_IDENT };
}

struct symbol_tbl;

struct symbol_tbl *symbol_table_new(struct symbol_table_attr *attr);
void symbol_table_free(struct symbol_tbl *tbl);
int symbol_table_update(struct symbol_tbl *tbl, struct symbol *symbol);
struct symbol *symbol_table_lookup(struct symbol_tbl *tbl, const char *ident,
				   size_t prefixlen);
