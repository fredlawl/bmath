#define _GNU_SOURCE
#include <asm-generic/errno-base.h>
#include <ctype.h>
#include <errno.h>
#include <ini.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "libbmath/src/print.h"

static enum cfg_file next_cfg = CFG_NONE;

char *locate_next_config_file(const char *override)
{
	size_t bytes;
	char *env;
	char *env_path;

	next_cfg++;
	switch (next_cfg) {
	case CFG_GLOBAL:
		return strdup("/etc/bmath/config.conf");
	case CFG_XDG:
		env = secure_getenv("XDG_CONFIG_HOME");
		if (!env) {
			return locate_next_config_file(override);
		}

		bytes = snprintf(NULL, 0, "%s/bmath/config.conf", env);
		env_path = calloc(bytes + 1, sizeof(*env));
		if (!env_path) {
			return locate_next_config_file(override);
		}

		snprintf(env_path, bytes + 1, "%s/bmath/config.conf", env);
		return env_path;
	case CFG_HOME:
		env = secure_getenv("HOME");
		if (!env) {
			return locate_next_config_file(override);
		}

		bytes = snprintf(NULL, 0, "%s/.config/bmath/config.conf", env);
		env_path = calloc(bytes + 1, sizeof(*env));
		if (!env_path) {
			return locate_next_config_file(override);
		}

		snprintf(env_path, bytes + 1, "%s/.config/bmath/config.conf",
			 env);
		return env_path;
	case CFG_RELATIVE:
		return strdup("./.bmath.conf");
	case CFG_OVERRIDE:
		if (!override) {
			return locate_next_config_file(override);
		}
		return strdup(override);
	default:
		break;
	}

	next_cfg = CFG_NONE;
	return NULL;
}

// The trim functions are GPT created. Sometimes, you just
// gotta. Trim is one of those functions C should have but
// doesnt for some reason...
char *ltrim(char *str)
{
	while (isspace((unsigned char)*str)) {
		str++;
	}
	return str;
}

char *rtrim(char *str)
{
	size_t len = strlen(str);
	while (len > 0 && isspace((unsigned char)str[len - 1])) {
		str[--len] = '\0';
	}
	return str;
}

char *trim(char *str)
{
	return rtrim(ltrim(str));
}

// Must free
ssize_t parse_encodings_list(const char *list, enum encoding_t **out)
{
	size_t i;
	char *token = NULL;
	char *saveptr = NULL;
	char *mutable_list = strdup(list);
	enum encoding_t visited[ENC_LENGTH + 1] = { 0 };
	enum encoding_t order[ENC_LENGTH + 1] = { 0 };
	struct enc_name *enc_shortname;
	ssize_t parsed = 0;
	*out = NULL;

	if (!mutable_list) {
		return -ENOMEM;
	}

	// all can only be used in isolation
	if (strlen(mutable_list) == sizeof("all") - 1 &&
	    !strcasecmp(mutable_list, "all")) {
		for (i = 1; i <= ENC_LENGTH; i++) {
			order[i - 1] = i;
		}
		parsed = ENC_LENGTH;
		goto put_encodings;
	}

	token = strtok_r(mutable_list, ",", &saveptr);
	while (token != NULL) {
		bool valid_choice = false;
		token = trim(token);

		for (i = 1; i <= ENC_LENGTH; i++) {
			if (visited[i]) {
				continue;
			}

			enc_shortname = enc_shortstr(i);
			if (!strncasecmp(enc_shortname->name, token,
					 enc_shortname->len)) {
				order[parsed] = i;
				visited[i] = 1;
				parsed++;
				valid_choice = true;
			}
		}

		if (!valid_choice) {
			parsed = -EINVAL;
			goto out;
		}

		token = strtok_r(NULL, ",", &saveptr);
	}

	if (parsed <= 0) {
		goto out;
	}

put_encodings:
	*out = calloc(parsed, sizeof(**out));
	if (!*out) {
		parsed = -ENOMEM;
		goto out;
	}

	memcpy(*out, order, parsed * sizeof(**out));

out:
	free(mutable_list);
	return parsed;
}

static int ini_strtobool(const char *value, bool *ret, bool _default)
{
	*ret = _default;
	if (!value) {
		return -EINVAL;
	}

	switch (*value) {
	case '1':
	case 'T':
	case 't':
	case 'y':
	case 'Y':
		*ret = true;
		return 0;
	case '0':
	case 'F':
	case 'f':
	case 'n':
	case 'N':
		*ret = false;
		return 0;
	}

	return -EINVAL;
}

static int handler(void *cfg, const char *section, const char *name,
		   const char *value)
{
#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
	struct config *config = (struct config *)cfg;
	ssize_t szret;
	bool boolret;

	if (MATCH("format", "human")) {
		if (ini_strtobool(value, &boolret, false)) {
			return 0;
		}

		if (boolret) {
			config->enc_fmt |= FMT_HUMAN;
		} else {
			config->enc_fmt &= ~FMT_HUMAN;
		}
	} else if (MATCH("format", "uppercase")) {
		if (ini_strtobool(value, &boolret, false)) {
			return 0;
		}

		if (boolret) {
			config->enc_fmt |= FMT_UPPERCASE;
		} else {
			config->enc_fmt &= ~FMT_UPPERCASE;
		}
	} else if (MATCH("format", "justify")) {
		if (ini_strtobool(value, &boolret, false)) {
			return 0;
		}

		if (boolret) {
			config->output_fmt |= OUT_FMT_JUSTIFY;
		} else {
			config->output_fmt &= ~OUT_FMT_JUSTIFY;
		}
	} else if (MATCH("format", "encodings")) {
		// free previous to overwrite with new
		free(config->encoding_order);
		config->encoding_order = NULL;

		szret = parse_encodings_list(value, &config->encoding_order);
		if (szret < 0) {
			return 0;
		}
		config->encoding_order_len = szret;
	} else {
		return 0; /* unknown section/name, error */
	}

	return 1;
}

int config_overwrite_from_file(const char *file, struct config *cfg)
{
	int err;
	FILE *cfg_file;

	if (!file) {
		return -EINVAL;
	}

	cfg_file = fopen(file, "r");
	if (!cfg_file) {
		err = errno;
		if (err == ENOENT) {
			return 0;
		}
		return -err;
	}

	err = ini_parse_file(cfg_file, handler, cfg);
	if (err) {
		fclose(cfg_file);
		if (err == -2) {
			// memory erro
			return -ENOMEM;
		}

		if (err == -1) {
			// -1 is file error, but doens't really tell us what the error is tho ?? see the docs again when you're less sleepy
			return -ENOENT;
		}

		// err returns line of first error
		return err;
	}

	fclose(cfg_file);
	return 0;
}
