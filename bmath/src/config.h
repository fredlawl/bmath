#pragma once

#include <stddef.h>
#include <stdlib.h>
#include <sys/types.h>

#include "libbmath/src/print.h"

enum cfg_file {
	CFG_NONE = 0,
	CFG_GLOBAL,
	CFG_XDG,
	CFG_HOME,
	CFG_RELATIVE,
	CFG_OVERRIDE
};

struct config {
	enum encoding_t *encoding_order;
	size_t encoding_order_len;
	enum format_t enc_fmt;
	enum output_format_t output_fmt;
};

struct config_defaults {
	const char *fmt_encodings;
	bool fmt_uppercase;
	bool fmt_human;
	bool fmt_justify;
};

// Must free
char *locate_next_config_file(const char *override);

// Must free
ssize_t parse_encodings_list(const char *list, enum encoding_t **out);

int config_overwrite_from_file(const char *file, struct config *cfg);

static inline void config_free(struct config *cfg)
{
	if (!cfg) {
		return;
	}

	if (cfg->encoding_order) {
		free(cfg->encoding_order);
	}

	free(cfg);
}

static inline struct config *config_new()
{
	ssize_t encoding_len;
	struct config *cfg = calloc(1, sizeof(struct config));
	if (!cfg) {
		return NULL;
	}

	cfg->enc_fmt = FMT_NONE;
	cfg->output_fmt = OUT_FMT_NONE;

	encoding_len = parse_encodings_list("uint", &cfg->encoding_order);
	if (encoding_len < 0) {
		config_free(cfg);
		return NULL;
	}
	cfg->encoding_order_len = encoding_len;

	return cfg;
}
