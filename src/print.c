#include <assert.h>
#include <iconv.h>
#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <asm-generic/errno-base.h>
#include <arpa/inet.h>

#include "print.h"

static enum bits_t hex_enc_to_bits_lookup[] = {
	[ENC_HEX] = BITS_MINIMAL,
	[ENC_HEX16] = BITS_16,
	[ENC_HEX32] = BITS_32,
	[ENC_HEX64] = BITS_64,
};

static enum bits_t utf_enc_to_bits_lookup[] = {
	[ENC_UTF8] = BITS_8,
	[ENC_UTF16] = BITS_16,
	[ENC_UTF32] = BITS_32,
};

static int justify_offsets[] = {
	[ENC_ASCII] = 6, // ascii:
	[ENC_BINARY] = 0,
	[ENC_HEX] = 4, // hex:
	[ENC_HEX16] = 6, // hex16:
	[ENC_HEX32] = 6, // hex32:
	[ENC_HEX64] = 6, // hex64:
	[ENC_INT] = 4, // i16: this breaks for i8
	[ENC_UINT] = 4, // u64:
	[ENC_OCTAL] = 4, // oct:
	[ENC_UNICODE] = 8, // unicode:
	[ENC_UTF8] = 8, // utf-8be:
	[ENC_UTF16] = 9, // utf-16be:
	[ENC_UTF32] = 9 // utf-32be:
};

ssize_t print_all(FILE *stream, uint64_t num, enum encoding_t encode_order[],
		  size_t encode_order_len, enum format_t fmt,
		  enum output_format_t output_fmt)
{
#define BUF_SIZE 128
	size_t i;
	int prefix;
	enum encoding_t enc;
	ssize_t bytes_written = 0;
	int longest_justify_offset = 0;
	bool do_justify = (fmt & FMT_HUMAN) && (output_fmt & OUT_FMT_JUSTIFY) &&
			  encode_order_len > 1;

	if (do_justify) {
		for (i = 0; i < encode_order_len; i++) {
			enc = encode_order[i];
			if (justify_offsets[enc] > longest_justify_offset) {
				longest_justify_offset = justify_offsets[enc];
			}
		}
	}

	for (i = 0; i < encode_order_len; i++) {
		enc = encode_order[i];
		char buff[BUF_SIZE] = { 0 };
		size_t written_bytes = 0;
		ssize_t bytes = 0;

		if (i > 0) {
			memset(buff, '\n', sizeof(char));
			bytes += sizeof(char);
		}

		if (do_justify && justify_offsets[enc]) {
			prefix = longest_justify_offset - justify_offsets[enc] +
				 (enc == ENC_INT && num <= UINT8_MAX);
			if (prefix) {
				bytes += snprintf(buff + bytes, BUF_SIZE, "%*c",
						  prefix, ' ');
			}
		}

		switch (enc) {
		case ENC_ASCII:
			bytes += ascii_str(buff + bytes, BUF_SIZE - bytes, num,
					   fmt);
			break;
		case ENC_BINARY:
			bytes += binary_str(buff + bytes, BUF_SIZE - bytes, num,
					    fmt);
			break;
		case ENC_INT:
		case ENC_UINT:
			bytes += int_str(buff + bytes, BUF_SIZE - bytes, num,
					 enc == ENC_UINT, fmt);
			break;
		case ENC_OCTAL:
			bytes += oct_str(buff + bytes, BUF_SIZE - bytes, num,
					 fmt);
			break;
		case ENC_HEX:
		case ENC_HEX16:
		case ENC_HEX32:
		case ENC_HEX64:
			bytes += hex_str(buff + bytes, BUF_SIZE - bytes, num,
					 hex_enc_to_bits_lookup[enc], fmt);
			break;
		case ENC_UNICODE:
			bytes += unicode_str(buff + bytes, BUF_SIZE - bytes,
					     num, fmt);
			break;
		case ENC_UTF8:
		case ENC_UTF16:
		case ENC_UTF32:
			bytes += utf_str(buff + bytes, BUF_SIZE - bytes, num,
					 utf_enc_to_bits_lookup[enc], fmt);
			break;
		default:
			return -EINVAL;
		}

		if (bytes < 0) {
			return -ENOMEM;
		}

		if (bytes == 0) {
			continue;
		}

		written_bytes = fwrite(buff, sizeof(char), bytes, stream);
		if (written_bytes < (size_t)bytes) {
			return -ENOMEM;
		}

		bytes_written += written_bytes;
	}

	return bytes_written;
}

ssize_t binary_str(char *dest, size_t dest_len, uint64_t number,
		   enum format_t fmt)
{
	size_t min_size = (sizeof(number) * 8) + 7;

	if (!dest) {
		return min_size;
	}

	if (dest_len < min_size) {
		return -EINVAL;
	}

	memset(dest, '0', min_size);

	// Print table can be pre-allocated
	dest[8] = ' ';
	dest[16 + 1] = ' ';
	dest[24 + 2] = ' ';
	dest[32 + 3] = '\n';
	dest[40 + 4] = ' ';
	dest[48 + 5] = ' ';
	dest[56 + 6] = ' ';

	int i = 0;
	while (number) {
		i += dest[min_size - i] != '0';
		dest[min_size - i] = (number & 0x1) + '0';
		number >>= 1;
		i++;
	}

	return min_size;
}

// TODO: Make this work with NULL dest like snprintf
ssize_t ascii_str(char *dest, size_t dest_len, uint64_t number,
		  enum format_t fmt)
{
	size_t min_size = sizeof("Ascii: ") - 1 + sizeof("<special>") - 1;
	size_t bytes = 0;

	if (dest_len < min_size) {
		return -EINVAL;
	}

	if (fmt & FMT_HUMAN) {
		memcpy(dest, "Ascii: ", sizeof("Ascii: ") - 1);
		bytes += sizeof("Ascii: ") - 1;
	}

	if (number <= CHAR_MAX) {
		if (number <= 31) {
			memcpy(dest + bytes, "<special>",
			       sizeof("<special>") - 1);
			bytes += sizeof("<special>") - 1;
		} else {
			memcpy(dest + bytes, (char *)&number, sizeof(char));
			bytes += 1;
		}
	} else {
		memcpy(dest + bytes, "Exceeded", sizeof("Excceded") - 1);
		bytes += sizeof("Exceeded") - 1;
	}

	return bytes;
}

ssize_t hex_str(char *dest, size_t dest_len, uint64_t number, enum bits_t bits,
		enum format_t fmt)
{
	char *prefix;
	char *sdfmt = (fmt & FMT_UPPERCASE) ? "%s0x%0*" PRIX64 :
					      "%s0x%0*" PRIx64;
	char *sfmt = (fmt & FMT_UPPERCASE) ? "%s0x%" PRIX64 : "%s0x%" PRIx64;
	int zeros = 2;

	switch (bits) {
	case BITS_16:
		prefix = (fmt & FMT_HUMAN) ? "Hex16: " : "";
		if (number > UINT16_MAX) {
			return snprintf(dest, dest_len, "%sExceeded", prefix);
		}
		zeros *= 2;
		break;
	case BITS_32:
		prefix = (fmt & FMT_HUMAN) ? "Hex32: " : "";
		if (number > UINT32_MAX) {
			return snprintf(dest, dest_len, "%sExceeded", prefix);
		}
		zeros *= 4;
		break;
	case BITS_64:
		prefix = (fmt & FMT_HUMAN) ? "Hex64: " : "";
		zeros *= 8;
		break;
	default:
		prefix = (fmt & FMT_HUMAN) ? "Hex: " : "";
		break;
	}

	if (zeros == 2) {
		return snprintf(dest, dest_len, sfmt, prefix, number);
	}

	return snprintf(dest, dest_len, sdfmt, prefix, zeros, number);
}

ssize_t int_str(char *dest, size_t dest_len, uint64_t number, bool is_unsigned,
		enum format_t fmt)
{
	if (is_unsigned) {
		return snprintf(dest, dest_len, "%s%" PRIu64,
				(fmt & FMT_HUMAN) ? "u64: " : "", number);
	}

	if (number <= UINT8_MAX) {
		return snprintf(dest, dest_len, "%s%" PRId8,
				(fmt & FMT_HUMAN) ? "i8: " : "",
				(int8_t)number);
	} else if (number <= UINT16_MAX) {
		return snprintf(dest, dest_len, "%s%" PRId16,
				(fmt & FMT_HUMAN) ? "i16: " : "",
				(int16_t)number);
	} else if (number <= UINT32_MAX) {
		return snprintf(dest, dest_len, "%s%" PRId32,
				(fmt & FMT_HUMAN) ? "i32: " : "",
				(int32_t)number);
	}

	return snprintf(dest, dest_len, "%s%" PRId64,
			(fmt & FMT_HUMAN) ? "i64: " : "", (int64_t)number);
}

ssize_t oct_str(char *dest, size_t dest_len, uint64_t number, enum format_t fmt)
{
	return snprintf(dest, dest_len, "%s0%" PRIo64,
			(fmt & FMT_HUMAN) ? "Oct: " : "", (uint64_t)number);
}

#define ICONV_ERR ((iconv_t) - 1)

static const char *to_encoding_lookup[] = { [BITS_8] = "UTF-8",
					    [BITS_16] = "UTF-16BE",
					    [BITS_32] = "UTF-32BE" };

static const char *from_encoding_lookup[] = { [BITS_8] = "UTF-8",
					      [BITS_16] = "UTF-16LE",
					      [BITS_32] = "UTF-32LE" };

static iconv_t
	iconv_descriptors[] = { [BITS_8] = 0, [BITS_16] = 0, [BITS_32] = 0 };
static bool iconv_setup = false;

static void unicode_teardown()
{
	iconv_close(iconv_descriptors[BITS_8]);
	iconv_close(iconv_descriptors[BITS_16]);
	iconv_close(iconv_descriptors[BITS_32]);
}

static void unicode_setup()
{
	int err;

	if (iconv_setup) {
		return;
	}

	iconv_setup = true;

	iconv_descriptors[BITS_8] = iconv_open(to_encoding_lookup[BITS_8],
					       from_encoding_lookup[BITS_32]);
	assert(iconv_descriptors[BITS_8] != ICONV_ERR);

	iconv_descriptors[BITS_16] = iconv_open(to_encoding_lookup[BITS_16],
						from_encoding_lookup[BITS_32]);
	assert(iconv_descriptors[BITS_16] != ICONV_ERR);

	iconv_descriptors[BITS_32] = iconv_open(to_encoding_lookup[BITS_32],
						from_encoding_lookup[BITS_32]);
	assert(iconv_descriptors[BITS_32] != ICONV_ERR);

	// It's better to keep around these pointers than to constantly
	// open/close. From a library implementation perspective the next
	// few lines suck, but I want valgrind to be happy.
	// I also don't want to implement a context for printing just
	// for iconv handling.
	err = atexit(unicode_teardown);
	assert(err == 0);
}

static ssize_t do_unicode_conversion(uint64_t number, enum bits_t bits,
				     char *output, size_t *output_size)
{
	iconv_t cd;
	size_t bytes_converted;
	char *input = (char *)&number;
	size_t input_size = sizeof(number);

	unicode_setup();
	if (number > UINT32_MAX) {
		return -E2BIG;
	}

	cd = iconv_descriptors[bits];
	bytes_converted = iconv(cd, &input, &input_size, &output, output_size);
	if (bytes_converted == (size_t)-1) {
		return -EINVAL;
	}

	return bytes_converted;
}

ssize_t utf_str(char *dest, size_t dest_len, uint64_t number, enum bits_t bits,
		enum format_t fmt)
{
	static size_t offset[] = { [BITS_8] = 1, [BITS_16] = 2, [BITS_32] = 4 };
	static const char *prefix_match[] = { [BITS_8] = "UTF-8BE: ",
					      [BITS_16] = "UTF-16BE: ",
					      [BITS_32] = "UTF-32BE: " };

	const char *prefix;
	ssize_t conversion;
	char output[8] = { 0 };
	size_t output_size = sizeof(output);
	const char *hex_fmt = (fmt & FMT_UPPERCASE) ? "%02" PRIX64 :
						      "%02" PRIx64;

	prefix = (fmt & FMT_HUMAN) ? prefix_match[bits] : "";

	if (number > UINT32_MAX) {
		return snprintf(dest, dest_len, "%sExceeded", prefix);
	}

	conversion = do_unicode_conversion(number, bits, output, &output_size);
	if (conversion < 0) {
		return snprintf(dest, dest_len, "%s<invalid>", prefix);
	}

	/*
		 re: offset
		 This kind of works because iconv decrements the output buffer size, and we
		 know based on UTF8-16 if there's 1-2 null bytes at the end of our buffer,
		 and with UTF32, the null character is 4 bytes.
	*/
	ssize_t bytes = 0;
	bytes += snprintf(dest, dest_len, "%s0x", prefix);
	for (size_t i = 0; i < 8 - (output_size + offset[bits]); i++) {
		bytes += snprintf(dest + bytes, dest_len - bytes, hex_fmt,
				  (uint64_t)0xff & output[i]);
	}

	return bytes;
}

ssize_t unicode_str(char *dest, size_t dest_len, uint64_t number,
		    enum format_t fmt)
{
	const char *prefix = (fmt & FMT_HUMAN) ? "Unicode: " : "";
	char output[8] = { 0 };
	size_t output_size = sizeof(output);
	ssize_t conversion;

	if (number < 31) {
		return snprintf(dest, dest_len, "%s<special>", prefix);
	}

	conversion =
		do_unicode_conversion(number, BITS_8, output, &output_size);
	if (conversion < 0) {
		return snprintf(dest, dest_len, "%s<invalid>", prefix);
	}

	return snprintf(dest, dest_len, "%s%s", prefix, output);
}
