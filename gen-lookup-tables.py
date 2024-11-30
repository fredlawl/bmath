#!/usr/bin/env python3


def map_data(table, mapdata):
    tbl = table.copy()
    for i, v in mapdata.items():
        tbl[i] = v
    return tbl


def gen_table(conditional):
    arr = [0] * 128
    for i in range(0, len(arr)):
        if conditional(i):
            arr[i] = 1
    return arr


def gen_lookup_table(label, funclabel, table):
    print(f"static int {label}[] = {{", end="")
    for i in range(0, len(table)):
        if i % 16 == 0:
            print("\n    ", end="")
        print(f"{table[i]}", end=", ", flush=True)

    print("\n};\n")
    print(f"static inline int {funclabel}(char c)\n{{")
    print(f"\treturn {label}[(int) c];")
    print("}")


print("// THIS FILE IS GENERATED!")
print("#ifndef LOOKUP_TABLES_H\n#define LOOKUP_TABLES_H\n")

# a-fA-F0-9
gen_lookup_table(
    "lookup_is_allowed_hex",
    "__is_allowed_hex",
    gen_table(
        lambda i: (i >= ord("a") and i <= ord("f"))
        or (i >= ord("A") and i <= ord("F"))
        or (i >= ord("0") and i <= ord("9"))
    ),
)
print()


gen_lookup_table(
    "lookup_hex_to_value",
    "__hex_to_value",
    map_data(
        gen_table(
            lambda i: (i >= ord("a") and i <= ord("f"))
            or (i >= ord("A") and i <= ord("F"))
            or (i >= ord("0") and i <= ord("9"))
        ),
        {
            ord("0"): 0,
            ord("1"): 1,
            ord("2"): 2,
            ord("3"): 3,
            ord("4"): 4,
            ord("5"): 5,
            ord("6"): 6,
            ord("7"): 7,
            ord("8"): 8,
            ord("9"): 9,
            ord("A"): 10,
            ord("a"): 10,
            ord("B"): 11,
            ord("b"): 11,
            ord("C"): 12,
            ord("c"): 12,
            ord("D"): 13,
            ord("d"): 13,
            ord("E"): 14,
            ord("e"): 14,
            ord("F"): 15,
            ord("f"): 15,
        },
    ),
)
print()


# 0-9
gen_lookup_table(
    "lookup_is_digit",
    "__is_digit",
    gen_table(lambda i: i >= ord("0") and i <= ord("9")),
)
print()

# A-F
gen_lookup_table(
    "lookup_is_capital_hex",
    "__is_capital_hex",
    gen_table(lambda i: i >= ord("A") and i <= ord("F")),
)
print()

# ()<> \t\n\rA-Za-z0-9
gen_lookup_table(
    "lookup_is_allowed_character",
    "__is_allowed_character",
    gen_table(
        lambda i: i == ord("~")
        or i == ord("(")
        or i == ord(")")
        or i == ord(">")
        or i == ord("<")
        or i == ord(" ")
        or i == ord("\t")
        or i == ord("\n")
        or i == ord("\r")
        or i == ord("|")
        or i == ord("&")
        or i == ord("~")
        or i == ord("^")
        or (i >= ord("A") and i <= ord("Z"))
        or (i >= ord("a") and i <= ord("z"))
        or (i >= ord("0") and i <= ord("9"))
    ),
)
print()

print("#endif")
