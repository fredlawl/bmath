#!/usr/bin/env python3

import random
import os
import sys

# Constraints
MIN_NUM_BYTES = 1
MAX_NUM_BYTES = 8


def gen_bytes():
    bc = random.randint(MIN_NUM_BYTES, MAX_NUM_BYTES)
    return random.randbytes(bc)


def gen_hex():
    return f"0x{gen_bytes().hex()}"


def gen_decimal():
    return f"{int.from_bytes(gen_bytes())}"


def gen_number():
    r = random.random()
    return gen_decimal() if r >= 0.7 else gen_hex()


def gen_factor():
    r = random.random()
    out = gen_number()

    if r >= 0.8:
        out = f"({gen_expr()})"

    r = random.random()
    if r >= 0.9:
        return f"~{out}"
    return out


def gen_term():
    op = [">>", "<<"]
    r = random.random()
    if r >= 0.3:
        return gen_factor()

    return f"{gen_term()} {op[int(r * 100) % 2]} {gen_factor()}"


def gen_expr():
    op = ["^", "|", "&"]
    r = random.random()
    if r >= 0.3:
        return gen_term()
    return f"{gen_expr()} {op[int(r * 100) % 3]} {gen_expr()}"


def main(argc, argv):
    random.seed(0)
    for i in range(0, 1000):
        sys.stdout.write(f"{gen_expr()}\n")
    sys.stdout.flush()
    return 0


if __name__ == "__main__":
    exit(main(len(sys.argv), sys.argv))
