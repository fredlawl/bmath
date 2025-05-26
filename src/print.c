#include <assert.h>
#include <iconv.h>
#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "print.h"

static const char *to_encoding_lookup[] = { [ENC_UTF8] = "UTF-8",
					    [ENC_UTF16] = "UTF-16BE",
					    [ENC_UTF32] = "UTF-32BE" };

static const char *from_encoding_lookup[] = { [ENC_UTF8] = "UTF-8",
					      [ENC_UTF16] = "UTF-16LE",
					      [ENC_UTF32] = "UTF-32LE" };

static const char *to_encoding_pretty_print_lookup[] = { [ENC_UTF8] = " UTF-8",
							 [ENC_UTF16] = "UTF-16",
							 [ENC_UTF32] =
								 "UTF-32" };

static iconv_t iconv_descriptors[ENC_UTF32 + 1] = { 0 };

static bool iconv_setup = false;

static void print_teardown()
{
	iconv_close(iconv_descriptors[ENC_UTF8]);
	iconv_close(iconv_descriptors[ENC_UTF16]);
	iconv_close(iconv_descriptors[ENC_UTF32]);
}

static void print_setup_unicode()
{
#define ICONV_ERR ((iconv_t) - 1)
	int err;
	iconv_setup = true;

	iconv_descriptors[ENC_UTF8] = iconv_open(
		to_encoding_lookup[ENC_UTF8], from_encoding_lookup[ENC_UTF32]);
	assert(iconv_descriptors[ENC_UTF8] != ICONV_ERR);

	iconv_descriptors[ENC_UTF16] = iconv_open(
		to_encoding_lookup[ENC_UTF16], from_encoding_lookup[ENC_UTF32]);
	assert(iconv_descriptors[ENC_UTF16] != ICONV_ERR);

	iconv_descriptors[ENC_UTF32] = iconv_open(
		to_encoding_lookup[ENC_UTF32], from_encoding_lookup[ENC_UTF32]);
	assert(iconv_descriptors[ENC_UTF32] != ICONV_ERR);

	// It's better to keep around these pointers than to constantly
	// open/close. From a library implementation perspective the next
	// few lines suck, but I want valgrind to be happy.
	// I also don't want to implement a context for printing just
	// for iconv handling.
	err = atexit(print_teardown);
	assert(err == 0);
}

static void print_unicode(uint64_t num, bool uppercase_hex,
			  enum encoding_t to_unicode)
{
	iconv_t cd;
	size_t conversion;

	char number_as_byte_array[8] = { 0 };
	char *utf8_input = number_as_byte_array;
	char *to_unicode_input = number_as_byte_array;
	size_t in_size = sizeof number_as_byte_array;
	size_t in_bytes_size = sizeof number_as_byte_array;

	char utf8_buf[8] = { 0 };
	char *utf8 = utf8_buf;
	char to_unicode_buf[8] = { 0 };
	char *to_unicode_bytes = to_unicode_buf;
	size_t utf8_size = sizeof utf8_buf;
	size_t to_unicode_size = sizeof to_unicode_buf;

	size_t offset[] = { [ENC_UTF8] = 1, [ENC_UTF16] = 2, [ENC_UTF32] = 4 };

	if (num > UINT32_MAX) {
		printf("%s: Exceeded\n",
		       to_encoding_pretty_print_lookup[to_unicode]);
		return;
	}

	memcpy(&number_as_byte_array, &num, sizeof num);

	printf("%s: ", to_encoding_pretty_print_lookup[to_unicode]);

	// Convert to UTF-8
	if (num < 31) {
		fputs("<special> ", stdout);
	} else {
		cd = iconv_descriptors[ENC_UTF8];
		conversion =
			iconv(cd, &utf8_input, &in_size, &utf8, &utf8_size);

		if (conversion == (size_t)-1) {
			fputs("<invalid> ", stdout);
		} else {
			printf("%s ", utf8_buf);
		}
	}

	// Convert from_unicode to to_unicode
	cd = iconv_descriptors[to_unicode];
	conversion = iconv(cd, &to_unicode_input, &in_bytes_size,
			   &to_unicode_bytes, &to_unicode_size);

	if (conversion == (size_t)-1) {
		puts("<invalid>");
		return;
	}

	fputs("(0x", stdout);

	/*
	 re: offset
	 This kind of works because iconv decrements the output buffer size, and we
	 know based on UTF8-16 if there's 1-2 null bytes at the end of our buffer,
	 and with UTF32, the null character is 4 bytes.
	 */
	for (size_t i = 0; i < 8 - (to_unicode_size + offset[to_unicode]);
	     i++) {
		__print_hex((uint64_t)0xff & to_unicode_buf[i], 2,
			    uppercase_hex);
	}

	puts(")");
}

void print_binary(uint64_t number)
{
	// (bytes * bits per byte) + 2 newlines + 6 spaces + 1 null
	char buff[(sizeof(number) * 8) + 8 + 1] = { 0 };
	memset(buff, '0', sizeof(buff) - 1);

	// Print table can be pre-allocated
	buff[8] = ' ';
	buff[16 + 1] = ' ';
	buff[24 + 2] = ' ';
	buff[32 + 3] = '\n';
	buff[40 + 4] = ' ';
	buff[48 + 5] = ' ';
	buff[56 + 6] = ' ';
	buff[64 + 7] = '\n';

	int i = 2;
	while (number) {
		i += buff[sizeof(buff) - i] != '0';
		buff[sizeof(buff) - i] = (number & 0x1) + '0';
		number >>= 1;
		i++;
	}

	fwrite(buff, sizeof(buff), 1, stdout);
}

void print_number(uint64_t num, bool uppercase_hex, int encoding_mask)
{
	if (encoding_mask == ENC_NONE) {
		return;
	}

	printf("   u64: %" PRIu64 "\n", num);

	if (num <= 0xff) {
		printf("    i8: %" PRId8 "\n", (int8_t)num);
	} else if (num <= 0xffff) {
		printf("   i16: %" PRId16 "\n", (int16_t)num);
	} else if (num <= 0xffffffff) {
		printf("   i32: %" PRId32 "\n", (int32_t)num);
	} else {
		printf("   i64: %" PRId64 "\n", (int64_t)num);
	}

	if (encoding_mask & ENC_ASCII) {
		if (num <= CHAR_MAX) {
			if (num <= 31) {
				puts("  char: <special>");
			} else {
				printf("  char: %c\n", (char)num);
			}
		} else {
			puts("  char: Exceeded");
		}
	}

	if (encoding_mask & ENC_UTF) {
		if (!iconv_setup) {
			print_setup_unicode();
		}

		if ((encoding_mask & ENC_UTF8) == ENC_UTF8) {
			print_unicode(num, uppercase_hex, ENC_UTF8);
		}

		if ((encoding_mask & ENC_UTF16) == ENC_UTF16) {
			print_unicode(num, uppercase_hex, ENC_UTF16);
		}

		if ((encoding_mask & ENC_UTF32) == ENC_UTF32) {
			print_unicode(num, uppercase_hex, ENC_UTF32);
		}
	}

	fputs("   Hex: 0x", stdout);
	__print_hex(num, 0, uppercase_hex);
	putchar('\n');

	if (num <= UINT16_MAX) {
		fputs(" Hex16: 0x", stdout);
		__print_hex(num, 4, uppercase_hex);
		putchar('\n');
	} else {
		puts(" Hex16: Exceeded");
	}

	if (num <= UINT32_MAX) {
		fputs(" Hex32: 0x", stdout);
		__print_hex(num, 8, uppercase_hex);
		putchar('\n');
	} else {
		puts(" Hex32: Exceeded");
	}

	fputs(" Hex64: 0x", stdout);
	__print_hex(num, 16, uppercase_hex);
	putchar('\n');
}
