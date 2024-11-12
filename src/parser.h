#ifndef BMATH_PARSER_H
#define BMATH_PARSER_H

#include <stdint.h>
#include <stddef.h>

/*
 * Parse error codes
 */
#define PE_EXPRESSION_TOO_LONG 1
#define PE_PARSE_ERROR 2
#define PE_NOTHING_TO_PARSE 3

#define P_MAX_EXP_LEN 512

/**
 * Convert infix notation to postfix notation. This takes care of parsing
 * operands and hex for operation.
 * @param const char *infix_expression
 * @param size_t len
 * @param uint64_t *out_result Result of the evaluation
 * @return Any positive integer means successful parse; a zero
 *         or negative value corresponds with a error code
 */
int parse(const char *infix_expression, size_t len, uint64_t *out_result);

#endif
