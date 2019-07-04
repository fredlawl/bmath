# Bmath (Bitwise math)
Online or native shell calculators either have weird syntax to follow, or no
syntax at all that allows nested calculations. Bmath solves these problems with
a one stop tool that does calculations and conversions.

There is no support for negative values. The maximum number a calculation may
produce is a unsigned 64-bit integer. Overflow is possible if an expression
calculation exceeds that number.

## Install

Run the following commands:
1. `cmake CMakeLists.txt`
1. `make`
1. `cp ./bin/bmath /usr/local/bin`

## Usage example

```shell
$ ./bmath "1 << 6"
  Dec: 64
 Char: @
  Hex: 0x40
Hex16: 0x0040
Hex32: 0x00000040
Hex64: 0x0000000000000040

```

Also supports hex conversions:

```shell
$ ./bmath "0x40"
  Dec: 64
 Char: @
  Hex: 0x40
Hex16: 0x0040
Hex32: 0x00000040
Hex64: 0x0000000000000040

```

```shell
$ ./bmath "0x1 | 0x2"
  Dec: 3
 Char: <special>
  Hex: 0x3
Hex16: 0x0003
Hex32: 0x00000003
Hex64: 0x0000000000000003

```

When a value is calculated greater than unsigned 16-bit integer:
```shell
$ ./bmath "1 << 17"
  Dec: 131072
 Char: Exceeded
  Hex: 0x20000
Hex16: Exceeded
Hex32: 0x00020000
Hex64: 0x0000000000020000

```

## Syntax

```
expr  = expr, op, term
      | term ;
term = term, shift_op, factor
     | factor ;
factor = number
       | lparen, expr, rparen
       | negate, factor ;
number = digit, { digit }
       | hex ;
digit = [0-9], { [0-9] } ;
hex = "0x", [0-9a-fA-F], { [0-9a-fA-F] } ;
shift_op = "<<" | ">>" ;
op = "^" | "|" | "&" ;
lparen = "(" ;
rparen = ")" ;
negate = "~" ;
```