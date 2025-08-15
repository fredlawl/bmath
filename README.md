# Bmath (Bitwise math)

Online or native shell calculators either have weird syntax to follow, or no
syntax at all that allows nested calculations. Bmath solves these problems with
a one stop tool that does calculations and conversions.

The maximum number a calculation may produce is a unsigned 64-bit integer.
Overflow is possible if an expression calculation exceeds that number.
This program assumes little-endian system, but big-endian input.

## Install

### Prerequisites

#### Linux

1. (test suite only) [Install Criterion](https://criterion.readthedocs.io/en/master/setup.html)

### Compile & Install

Clone this repo, `cd` into the cloned directory, and then run the following commands:

1. `meson setup --buildtype=release build`
2. `sudo meson install -C build --tags runtime,man`

## Usage

```
bmath [-a <EXPRESSION>] [-b] [-u] [--unicode] [EXPRESSION]
bmath [--help]
bmath [--usage]
bmath [-V]
```

Run `./bmath` to run the program in interactive mode. To exit, use `ctrl + c`,
or type in "quit" or "exit".

Pass a positional argument to run in
non-interactive mode:

```sh
bmath "1"
```

Optionally, pipe stdin:

```sh
bmath < /path/to/file
echo "1" | bmath
```

### Examples

```sh
bmath "1 << 6"
  Dec: 64
 Char: @
  Hex: 0x40
Hex16: 0x0040
Hex32: 0x00000040
Hex64: 0x0000000000000040
```

Also supports hex conversions:

```sh
bmath "0x40"
  Dec: 64
 Char: @
  Hex: 0x40
Hex16: 0x0040
Hex32: 0x00000040
Hex64: 0x0000000000000040
```

When a value is calculated greater than unsigned 16-bit integer:

```sh
bmath "1 << 17"
  Dec: 131072
 Char: Exceeded
  Hex: 0x20000
Hex16: Exceeded
Hex32: 0x00020000
Hex64: 0x0000000000020000
```

## Syntax

```
expr = signed, op, signed
     | signed ;
signed = number
       | lparen, expr, rparen
       | { logic_not | sign }, signed
       | function
function = function_name, lparen, expr, {",", expr }, rparen
number = digit, { digit }
       | hex ;
digit = [0-9], { [0-9] } ;
hex = "0x", [0-9a-fA-F], { [0-9a-fA-F] } ;
op = "|" | "^" | "&" | "<<" | ">>" | "-" | "+" | "*" | "%" ;
lparen = "(" ;
rparen = ")" ;
logic_not = "~" ;
sign = "-" | "+" ;

Functions:
align(x, align_to)
    Aligns x to align_to. align_to should be a power of two, but is not
    enforced.

align_down(x, align_to)
    Same as align() except the result is rounded down to nearest alginment.

bswap(x)
    Swaps the byte order of x. 16, 32, and 64 modes are supported respectfully.

mask(num_bytes)
    Creates a mask of num_bytes. num_bytes must be in range of [0, 8].

popcnt(x)
    Counts the number of 1's set in x.

Order of operations:
+------------+
| 1 | *, %   |
+------------+
| 2 | +, -   |
+---+--------+
| 3 | <<, >> |
+---+--------+
| 4 | &      |
+---+--------+
| 5 | ^      |
+---+--------+
| 6 | |      |
+---+--------+
```
