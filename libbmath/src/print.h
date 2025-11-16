#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

enum encoding_t {
	ENC_NONE = 0,
	ENC_ASCII,
	ENC_BINARY,
	ENC_HEX,
	ENC_HEX16,
	ENC_HEX32,
	ENC_HEX64,
	ENC_INT,
	ENC_UINT,
	ENC_OCTAL,
	ENC_UNICODE,
	ENC_UTF8,
	ENC_UTF16,
	ENC_UTF32,
};

#define ENC_LENGTH ENC_UTF32

enum bits_t {
	BITS_MINIMAL = 0,
	BITS_8,
	BITS_16,
	BITS_32,
	BITS_64,
};

enum format_t {
	FMT_NONE = 0,
	FMT_HUMAN = 1 << 0,
	FMT_UPPERCASE = 1 << 1,
};

enum output_format_t {
	OUT_FMT_NONE = 0,
	OUT_FMT_JUSTIFY = 1 << 0,
};

static struct enc_name {
	const char *name;
	size_t len;
} enc_shortname_lookup[] = { [ENC_ASCII] = { "ascii", sizeof("ascii") - 1 },
			     [ENC_BINARY] = { "binary", sizeof("binary") - 1 },
			     [ENC_HEX] = { "hex", sizeof("hex") - 1 },
			     [ENC_HEX16] = { "hex16", sizeof("hex16") - 1 },
			     [ENC_HEX32] = { "hex32", sizeof("hex32") - 1 },
			     [ENC_HEX64] = { "hex64", sizeof("hex64") - 1 },
			     [ENC_INT] = { "int", sizeof("int") - 1 },
			     [ENC_UINT] = { "uint", sizeof("uint") - 1 },
			     [ENC_OCTAL] = { "oct", sizeof("oct") - 1 },
			     [ENC_UNICODE] = { "unicode",
					       sizeof("unicode") - 1 },
			     [ENC_UTF8] = { "utf8", sizeof("utf8") - 1 },
			     [ENC_UTF16] = { "utf16", sizeof("utf16") - 1 },
			     [ENC_UTF32] = { "utf32", sizeof("utf32") - 1 } };

static inline struct enc_name *enc_shortstr(enum encoding_t encoding)
{
	return &enc_shortname_lookup[encoding];
}

ssize_t print_all(FILE *stream, uint64_t num, enum encoding_t encode_order[],
		  size_t encode_order_len, enum format_t fmt,
		  enum output_format_t output_fmt);

ssize_t binary_str(char *dest, size_t dest_len, uint64_t number,
		   enum format_t fmt);

ssize_t ascii_str(char *dest, size_t dest_len, uint64_t number,
		  enum format_t fmt);

ssize_t hex_str(char *dest, size_t dest_len, uint64_t number, enum bits_t bits,
		enum format_t fmt);

ssize_t int_str(char *dest, size_t dest_len, uint64_t number, bool is_unsigned,
		enum format_t fmt);

ssize_t oct_str(char *dest, size_t dest_len, uint64_t number,
		enum format_t fmt);

ssize_t unicode_str(char *dest, size_t dest_len, uint64_t number,
		    enum format_t fmt);

ssize_t utf_str(char *dest, size_t dest_len, uint64_t number, enum bits_t bits,
		enum format_t fmt);
