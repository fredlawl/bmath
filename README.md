# Bmath (Bitwise math)

Online or native shell calculators either have weird syntax to follow, or no
syntax at all that allows nested calculations. Bmath solves these problems with
a one stop tool that does calculations and conversions.

The maximum number a calculation may produce is a unsigned 64-bit integer.
Overflow is possible if an expression calculation exceeds that number.
This program assumes little-endian system, but big-endian input.

<!--toc:start-->
- [Bmath (Bitwise math)](#bmath-bitwise-math)
  - [Install](#install)
    - [Prerequisites](#prerequisites)
      - [Linux](#linux)
    - [Compile & Install](#compile-install)
  - [Usage](#usage)
    - [Configuration](#configuration)
      - [Supported encodings](#supported-encodings)
    - [Examples](#examples)
  - [Syntax](#syntax)
<!--toc:end-->

## Install

### Prerequisites

#### Linux

1. libreadline
2. libtinfo
3. libinih
4. [Install Unity](https://github.com/ThrowTheSwitch/Unity/tree/master) (tests only!)

### Compile & Install

Clone this repo, `cd` into the cloned directory, and then run the following commands:

1. `meson setup --buildtype=release build`
2. `meson compile -C build`
3. `meson test -C build` (if you want to run the tests)
4. `sudo meson install -C build --tags runtime,man`

## Usage

```
bmath [-c <FILE>] [-e <ENCODINGS>] [--fmt-human] [--fmt-justify] [--fmt-uppercase] [EXPRESSION]
bmath [-c <FILE>] [-e <ENCODINGS>] [--fmt-human] [--fmt-justify] [--fmt-uppercase] -w <FILE> 
bmath [--help]
bmath [--usage]
bmath [-V]
```

Run `./bmath` to run the program in interactive mode. To exit, use `ctrl + c`,
or type in "quit" or "exit".

Pass a positional argument to run in headless mode:

```sh
bmath "1"
```

Optionally, pipe stdin:

```sh
bmath < /path/to/file
#or
echo "1" | bmath
```

Live editing:

```sh
bmath -w /path/to/file
vim /path/to/file
```

### Configuration

Instead of only supplying command line arguments, bmath supports
a configuration file to set your own defaults.

Configuration files are searched & loaded in this order:

1. `/etc/bmath/config.conf`
2. `$XDG_CONFIG_HOME/bmath/config.conf`
3. `$HOME/.config/bmath/config.conf`
4. `./.bmath.conf`
5. `--config /path/to/config`

Passed command line arguments take overwrite cfg from files.

#### Supported encodings

```sh
# Binary
bmath -e "binary" 0xab
00000000 00000000 00000000 00000000
00000000 00000000 00000000 10101011
 
# Character
bmath -e "ascii" 0xff
Exceeded 

bmath -e "ascii" 64
@

bmath -e "ascii" 0x2
<special>

# Hex 
bmath -e "hex" 0xab
0xab
 
bmath -e "hex16" 0xab
0x00ab

bmath -e "hex32" 0xab
0x000000ab

bmath -e "hex64" 0xab
0x00000000000000ab

# Unsigned integer
bmath -e "uint" 0xab
171

# Signed integer
bmath -e "int" 0xab
-85

# Unicode
bmath -e "unicode" 0xab
«

bmath -e "utf8" 0xab
0xc2ab

bmath -e "utf16" 0xab
0x00ab

bmath -e "utf32" 0xab
0x000000ab
```

Supplement with `--fmt-human` to make encodings more human readable:

```sh
bmath --fmt-human -e "hex16" 0xab
Hex16: 0x00ab
```

Mix & match encoding options:

```sh
bmath -e "binary,hex,int,utf8,unicode" 0xab
00000000 00000000 00000000 00000000
00000000 00000000 00000000 10101011
0xab
-85
0xc2ab
«
```

Supplement with `--fmt-justify` to align results to the
longest prefix:

```sh
bmath --fmt-justify --fmt-human -e "binary,hex,int,utf8,unicode" 0xab
00000000 00000000 00000000 00000000
00000000 00000000 00000000 10101011
    Hex: 0xab
     i8: -85
UTF-8BE: 0xc2ab
Unicode: «
```

`--fmt-justify` only makes sense in the context of `--fmt-human`.

`binary` does not have a justify or human readable form because
it's fairly obvious what it is, long, and takes up multiple lines.

For the default format, see: [etc/bmath/config.conf](etc/bmath/config.conf)

### Examples

Functions:

```sh
bmath -e "binary" "8"
00000000 00000000 00000000 00000000
00000000 00000000 00000000 00001000

bmath -e "int" "clz(8, 1)"
4
```

Variables:

```sh
bmath -e "binary" @shift = 8; @1 = 1; @1 << @shift"
00000000 00000000 00000000 00000000
00000000 00000000 00000001 00000000
```

## Syntax

```
assignment = variable, "=", expr, ";" ;
expr = signed, op, signed
     | signed ; 
signed = number
       | "(", expr, ")"
       | { "~" | sign }, signed
       | function
       | variable ; 
function = ident, "(", expr, {",", expr }, ")" ;
number = digit, { digit }
       | hex ;
variable = "@", ident ;
digit = [0-9], { [0-9] } ;
hex = "0x", [0-9a-fA-F], { [0-9a-fA-F] } ;
ident = [_0-9a-fA-F], { [_0-9a-fA-F] } ;
op = "|" | "^" | "&" | "<<" | ">>" | "-" | "+" | "*" | "/" | "%" ;
sign = "-" | "+" ;

Functions:
align(x, align_to)
    Aligns x to align_to. align_to should be a power of two, but is not
    enforced.

align_down(x, align_to)
    Same as align() except the result is rounded down to nearest alginment.

bswap(x)
    Swaps the byte order of x. 16, 32, and 64 modes are supported respectfully.

clz(x, num_bytes)
    Counts number of zeros before the first 1, according to num_bytes.
    num_bytes must be in range of [1, 8].
    Example:
      x = 1, num_bytes = 8, expect result 63. If num_bytes = 1, expect 7.

ctz(x)
    Counts number of zeros after the last 1.
    Example:
      x = 1, expect result 0. If x = (1 << 63), expect 63.

mask(num_bytes)
    Creates a mask of num_bytes. num_bytes must be in range of [0, 8].

popcnt(x)
    Counts the number of 1's set in x.

Order of operations:
+-------------+
| 1 | *, /, % |
+-------------+
| 2 | +, -    |
+---+---------+
| 3 | <<, >>  |
+---+---------+
| 4 | &       |
+---+---------+
| 5 | ^       |
+---+---------+
| 6 | |       |
+---+---------+
```
