#include <arpa/inet.h>
#include <asm-generic/errno-base.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/param.h>

#include "symbol.h"

struct symbol_tbl_node {
	struct symbol_tbl_node *children[2];
	struct symbol *sym;
	bool intermediate;
	size_t prefixlen;
	char key[];
};

struct symbol_tbl {
	struct symbol_tbl_node *root;
	size_t maxprefixlen;
	size_t data_size;
};

#ifdef DEBUG
static void print_symbol_table_node(struct symbol_tbl_node *n, int child,
				    int level)
{
	if (!n) {
		return;
	}

	for (int i = 0; i < level; i++) {
		fputs("  ", stderr);
	}

	if (n->intermediate) {
		fprintf(stderr,
			"level: %d; child: %d; key: %s; prefix: %zu; (intermediate)\n",
			level, child, n->key, n->prefixlen);
	} else {
		fprintf(stderr,
			"level: %d; child: %d; key: %s; prefix: %zu; sym: %s; symlen: %zu\n",
			level, child, n->key, n->prefixlen,
			symbol_ident(n->sym), n->sym->ident_len);
	}
	level++;

	for (size_t i = 0; i < sizeof(n->children) / sizeof(n->children[0]);
	     i++) {
		print_symbol_table_node(n->children[i], i, level);
	}
}

static void print_symbol_table(struct symbol_tbl *n)
{
	if (!n) {
		return;
	}

	fprintf(stderr, "table:\n");
	print_symbol_table_node(n->root, 0, 0);
}
#else
static inline void print_symbol_table(struct symbol_tbl *n)
{
}
static inline void print_symbol_table_node(struct symbol_tbl_node *n, int child,
					   int level)
{
}
#endif

struct symbol_tbl *symbol_table_new(struct symbol_table_attr *attr)
{
	struct symbol_tbl *n;
	n = calloc(1, sizeof(*n) + attr->key_size);
	if (!n) {
		return NULL;
	}

	n->data_size = attr->key_size;
	n->maxprefixlen = attr->key_size * 8;
	return n;
}

static struct symbol_tbl_node *symbol_tbl_new_node(struct symbol_tbl *tbl)
{
	struct symbol_tbl_node *new_node;
	new_node = calloc(1, sizeof(*new_node) + tbl->data_size);
	if (!new_node) {
		return NULL;
	}
	return new_node;
}

static void symbol_tbl_node_free(struct symbol_tbl_node *n)
{
	if (!n) {
		return;
	}

	if (n->sym) {
		symbol_free(n->sym);
	}

	free(n);
}

void symbol_table_free(struct symbol_tbl *tbl)
{
	if (!tbl) {
		return;
	}

	print_symbol_table(tbl);

	for (size_t i = 0;
	     i < sizeof(tbl->root->children) / sizeof(tbl->root->children[0]);
	     i++) {
		symbol_tbl_node_free(tbl->root->children[i]);
	}

	symbol_tbl_node_free(tbl->root);
	free(tbl);
}

static inline int fls(unsigned int x)
{
	return x ? sizeof(x) * 8 - __builtin_clz(x) : 0;
}

static size_t longest_prefix_match(struct symbol_tbl *tbl,
				   struct symbol_tbl_node *node,
				   const char *key, size_t key_prefixlen)
{
	size_t prefixlen = 0, i = 0;
	size_t limit = MIN(node->prefixlen, key_prefixlen);

	while (tbl->data_size >= i + 4) {
		uint32_t diff = ntohl(*(uint32_t *)&node->key[i] ^
				      *(uint32_t *)&key[i]);
		prefixlen += 32 - fls(diff);
		if (prefixlen >= limit) {
			return limit;
		}

		if (diff) {
			return prefixlen;
		}

		i += 4;
	}

	if (tbl->data_size >= i + 2) {
		uint16_t diff = ntohs(*(uint16_t *)&node->key[i] ^
				      *(uint16_t *)&key[i]);
		prefixlen += 16 - fls(diff);
		if (prefixlen >= limit) {
			return limit;
		}

		if (diff) {
			return prefixlen;
		}

		i += 2;
	}

	if (tbl->data_size >= i + 1) {
		prefixlen += 8 - fls(node->key[i] ^ key[i]);
		if (prefixlen >= limit) {
			return limit;
		}
	}

	return prefixlen;
}

int extract_child(const char *key, size_t prefixlen)
{
	return !!(key[prefixlen / 8] & (1 << (7 - (prefixlen % 8))));
}

struct symbol *symbol_table_lookup(struct symbol_tbl *tbl, const char *ident,
				   size_t ident_len)
{
	struct symbol_tbl_node *node = NULL;
	struct symbol_tbl_node *found = NULL;
	char key[32] = { 0 };
	size_t key_prefixlen = ident_len * 8;

	memcpy(key, ident, ident_len);

	if (key_prefixlen > tbl->maxprefixlen) {
		return NULL;
	}

	for (node = tbl->root; node;) {
		unsigned int next_child;
		size_t matchlen;

		matchlen = longest_prefix_match(tbl, node, key, key_prefixlen);
		if (matchlen == tbl->maxprefixlen) {
			found = node;
			break;
		}

		if (matchlen < node->prefixlen) {
			break;
		}

		if (!node->intermediate) {
			found = node;
		}

		next_child = extract_child(key, node->prefixlen);
		node = node->children[next_child];
	}

	if (!found) {
		return NULL;
	}

	return found->sym;
}

// for now, the prefix is the tok.namelen
int symbol_table_update(struct symbol_tbl *tbl, struct symbol *sym)
{
	struct symbol_tbl_node *node;
	struct symbol_tbl_node *new_node;
	struct symbol_tbl_node **slot;
	struct symbol_tbl_node *intermediate;
	struct symbol_tbl_node *free_node = NULL;
	size_t matchlen = 0;
	int next_child;

	char key[32] = { 0 };
	size_t key_prefix = sym->ident_len * 8;

	memcpy(key, symbol_ident(sym), sym->ident_len);

	new_node = symbol_tbl_new_node(tbl);
	if (!new_node) {
		return EINVAL;
	}
	new_node->sym = sym;
	new_node->prefixlen = key_prefix;
	new_node->intermediate = false;
	memcpy(new_node->key, key, sym->ident_len);

	slot = &tbl->root;

	while ((node = *slot)) {
		matchlen = longest_prefix_match(tbl, node, key, key_prefix);
		if (node->prefixlen != matchlen ||
		    node->prefixlen == key_prefix) {
			break;
		}

		next_child = extract_child(key, node->prefixlen);
		slot = &node->children[next_child];
	}

	// empty root or free ptr
	if (!node) {
		*slot = new_node;
		goto out;
	}

	// if exists, replace
	if (node->prefixlen == matchlen) {
		new_node->children[0] = node->children[0];
		new_node->children[1] = node->children[1];
		*slot = new_node;
		free_node = node;
		goto out;
	}

	if (matchlen == key_prefix) {
		next_child = extract_child(node->key, matchlen);
		new_node->children[next_child] = node;
		*slot = new_node;
		goto out;
	}

	intermediate = symbol_tbl_new_node(tbl);
	if (!intermediate) {
		symbol_tbl_node_free(intermediate);
		return ENOMEM;
	}

	intermediate->prefixlen = matchlen;
	intermediate->intermediate = true;
	memcpy(intermediate->key, node->key, tbl->data_size);

	if (extract_child(key, matchlen)) {
		intermediate->children[0] = node;
		intermediate->children[1] = new_node;
	} else {
		intermediate->children[0] = new_node;
		intermediate->children[1] = node;
	}

	*slot = intermediate;

out:
	symbol_tbl_node_free(free_node);
	return 0;
}
