#!/usr/bin/env python3

import random
import os
import sys
import argparse
from bisect import bisect_left, bisect

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


def gen_signed(depth, ops, ops_weights):
    r = random.random()
    out = gen_number()

    if depth >= 4:
        return out

    if r >= 0.3:
        out = f"({gen_expr(depth + 1, ops, ops_weights)})"

    r = random.random()
    if r >= 0.999:
        return f"+{out}"

    if r >= 0.99:
        return f"~{out}"

    if r >= 0.9:
        return f"-{out}"

    return out


def calc_op_weights():
    op = ["^", "&", "|", "<<", ">>", "+", "-", "*", "%"]
    weights = [0.60, 0.70, 0.80, 0.30, 0.20, 0.10, 0.10, 0.05, 0.005]
    weights_sum = sum(weights)

    for i in range(0, len(weights)):
        weights[i] /= weights_sum

    for i in range(1, len(weights)):
        weights[i] += weights[i - 1]

    # print(weights)
    return (op, weights)


def gen_expr(depth, ops, ops_weights):
    r = random.random()
    idx = bisect_left(ops_weights, r)

    r2 = random.random()
    if r2 >= 0.4:
        return gen_signed(depth + 1, ops, ops_weights)
    return f"{gen_expr(depth + 1, ops, ops_weights)} {ops[idx]} {gen_signed(depth + 1, ops, ops_weights)}"


def main(args):
    random.seed(args.seed)
    ops, ops_weights = calc_op_weights()
    for i in range(0, args.iterations):
        sys.stdout.write(f"{gen_expr(0, ops, ops_weights)}\n")
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
