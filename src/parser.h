#ifndef BMATH_PARSER_H
#define BMATH_PARSER_H

#include <stdint.h>

/*
 * Parse error codes
 */
#define PE_NOTHING_TO_PARSE 0
#define PE_EXPRESSION_TOO_LONG 1

/**
 * Convert infix notation to postfix notation. This takes care of parsing
 * operands and hex for operation.
 * @param const char *infix_expression Assumes a non-null string
 * @param uint64_t *out_result Result of the evaluation
 * @return Any positive integer means successful parse; a zero
 *         or negative value corresponds with a error code
 */
int16_t parse(const char *infix_expression, uint64_t *out_result);

#endif
