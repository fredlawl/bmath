#!/usr/bin/env python3

import random
import os
import sys
import argparse

# Constraints
MIN_NUM_BYTES = 1
MAX_NUM_BYTES = 8


def gen_bytes():
    bc = int(random.random() * 1000)
    bc = max(bc % MAX_NUM_BYTES, MIN_NUM_BYTES)
    return random.randbytes(bc)


def gen_hex():
    return f"0x{gen_bytes().hex()}"


def gen_decimal():
    return f"{int.from_bytes(gen_bytes())}"


def gen_number():
    r = random.random()
    return gen_decimal() if r >= 0.4 else gen_hex()


def gen_factor(depth):
    r = random.random()
    out = gen_number()

    if depth >= 4:
        return out

    if r >= 0.3:
        out = f"({gen_expr(depth + 1)})"

    r = random.random()
    if r >= 0.9:
        return f"~{out}"
    return out


def gen_term(depth):
    op = [">>", "<<"]
    r = random.random()
    if r >= 0.3:
        return gen_factor(depth)

    return f"{gen_term(depth + 1)} {op[int(r * 100) % len(op)]} {gen_factor(depth)}"


def gen_expr(depth):
    op = ["^", "|", "&"]
    r = random.random()
    if r >= 0.4:
        return gen_term(depth + 1)
    return f"{gen_expr(depth + 1)} {op[int(r * 100) % len(op)]} {gen_term(depth + 1)}"


def main(args):
    random.seed(args.seed)
    for i in range(0, args.iterations):
        sys.stdout.write(f"{gen_expr(0)}\n")
    sys.stdout.flush()
    return 0


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Generate problems to perf test bmath."
    )
    parser.add_argument(
        "-i", "--iterations", help="Number of problems to generate", default=1, type=int
    )
    parser.add_argument("-s", "--seed", help="Seed", type=int, default=0)
    args = parser.parse_args()
    exit(main(args))
