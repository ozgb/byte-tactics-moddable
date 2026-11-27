"""Write and check the kit's runtime vectors, tests/runtime/vectors/ (#1060).

    uv run tools/runtimevectors.py write     # needs Wine, clang-cl and the vectors of tools/bitexact.py
    uv run tools/runtimevectors.py check     # the committed directory, standard library only
    uv run tools/runtimevectors.py readme    # rewrite README.md from vectors.json, after a change to its text
    uv run tools/test_runtimevectors.py      # tests, on fixture directories

`write` runs `tools/bitexact.py --kit DIR`, which replays the original's recorded results and writes every case
(its inputs and the recorded outputs) of each runtime entry to DIR as `<entry>.edge.bin`, `<entry>.random.bin` and
`<entry>.layout.txt`. It then writes tests/runtime/vectors/: one `<entry>.bin` per entry (the edge records, then a
sample of the random records), `vectors.json` (the manifest), `layouts.txt` (one line per entry: its name, its input
types and its output types, comma separated, for readers without a JSON parser) and `README.md` (the record format,
the sampling rule and one line per entry). The directory is exported to the data kit as `runtime/vectors/`.

The directory stays within BUDGET bytes. The edge records of every entry are kept whole, and the random records of
every entry are sampled to RANDOM_CAP bytes by a seeded rule (`strata`).

`check` fails (exit 1, one line per problem) when the manifest does not parse, a file's size is not its records'
sizes, a count differs from the manifest, the budget is exceeded, a file is not in the manifest, the README does
not name every entry, or the README is not the text `readme` writes for the manifest.
"""

import argparse
import json
import mmap
import os
import re
import struct
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "tests/runtime/vectors"
SCRATCH = ROOT / "build/scratch/runtimevectors"

BUDGET = 8 * 1024 * 1024
RANDOM_CAP = 80_000
MASK = (1 << 64) - 1

LIBRARY = ("sine", "cosine", "mul_hi", "scaled_sine", "scaled_cosine", "angle_of_vector", "rotate_point",
           "rotation_matrix", "apply_rotation", "mul_div", "sqrt", "sqrt_float", "hypot", "acos", "float_text",
           "double_text", "rand", "shuffle", "load_sequence")
OMITTED = {"qsort": "game logic has no qsort call, and the order in which equal elements end is left open"}
MEMORY_DEPENDENT = ("isspace", "isdigit", "isalnum")
MANIFEST = "vectors.json"
LAYOUTS = "layouts.txt"
README = "README.md"
FIXED = {"i32": 4, "u32": 4, "f64": 8}
BYTES = re.compile(r"bytes([1-9][0-9]*)")

DESCRIPTIONS = {
    "sine": "the sine of a 16-bit angle (65536 is a full turn) as a fixed-point int, 8192 being 1.0: the entry of the sine table that \"Sine table\" (Rules) gives; every angle is an edge record",
    "cosine": "the cosine of a 16-bit angle in the same fixed point: the entry of the sine table that \"Sine table\" (Rules) gives; every angle is an edge record",
    "mul_hi": "the high 32 bits of the signed 64-bit product of two ints",
    "scaled_sine": "scale times the sine of an angle, as an int (Rules, \"Scaled sine and cosine\")",
    "scaled_cosine": "scale times the cosine of an angle, as an int (Rules, \"Scaled sine and cosine\")",
    "angle_of_vector": "the angle (32768 is half a turn) of the vector whose inputs are, in record order, x and z: the arctangent of x over z (Rules)",
    "rotate_point": "the point (x, z) rotated by an angle: the inputs are `angle`, `x` and `z`, the outputs `rx` and `rz` (Rules)",
    "rotation_matrix": "the 3x3 rotation matrix m0 to m8 built from three angles (Rules)",
    "apply_rotation": "a vector (x, y, z) times the transpose of a 3x3 matrix m0 to m8, giving (rx, ry, rz); `aliased` is 1 when each result replaces its input before the next is computed (Rules). A record's inputs are m0 to m8 in that order (the matrix row by row, as \"Rotation matrix\" names its entries), then `x`, `y`, `z` and `aliased`; its outputs are `rx`, `ry` and `rz`, as `vectors.json` lists them",
    "mul_div": "a times b divided by c, the product taken in 64 bits and the quotient truncated toward zero, 0 when c is 0; a case whose quotient does not fit an int is not recorded",
    "sqrt": "the square root of a non-negative double",
    "sqrt_float": "the square root of a non-negative float, given and returned as its 32-bit pattern",
    "hypot": "the length of the vector (x, y), finite arguments (Rules)",
    "acos": "the arccosine of x in [-1, 1] as a double (Rules); the 7 cases whose recorded result differs from the rule are dropped",
    "float_text": "the float the C runtime's scanf conversion `%f` stores for a text, as the float's 32-bit pattern; it is not always the correctly rounded float (Rules, \"Text to a number\")",
    "double_text": "the double the C runtime's atof returns for a text: leading white space skipped, any trailing text ignored, overflow to infinity, underflow to zero or a denormal (Rules, \"Text to a number\")",
    "rand": "the first 10000 values of the C runtime's rand after srand(seed), as 10000 little endian i32 (40000 bytes)",
    "shuffle": "the ints 1, 4, 7, ... (3 * index + 1) for size elements after the C++ library's shuffle of a range, drawing from rand seeded by `seed`, as `size` little endian i32 behind a u32 byte count, then the next four values of rand",
    "load_sequence": "the load sequence: srand(seed), the 391606 rand draws the explosion setup makes, then the same shuffle of the first `size` (3 to 10) of the ints 0 to 9, as 10 little endian i32, then the next four values of rand",
    "atoi": "atoi of a string: white space of every kind, signs, leading zeros, trailing text, values around and past the int range and 2^32, high bytes",
    "itoa": "_itoa of a value in a radix: the text it writes",
    "strcmpi": "_strcmpi of two strings: the result (-1, 0 or 1)",
    "strnicmp": "_strnicmp of two strings and a count: the result (-1, 0 or 1)",
    "strncmp": "strncmp of two strings and a count: the result (-1, 0 or 1)",
    "compare_bytes": "every pair of strings of at most one byte: for the first string's byte `a` (0 to 255, 0 being the empty string), the results of _strcmpi, of _strnicmp with the counts 1 and 2 and of strncmp with the counts 1 and 2 against the second string of each byte b from 0 to 255, each output holding one signed byte (-1, 0 or 1) per b in order",
    "tolower": "tolower of an int from -128 to 255",
    "toupper": "toupper of an int from -128 to 255",
    "isspace": "isspace of an int from -1 to 255, the raw result: 8 for the bytes 9 to 13 and 32, else 0; an int from -128 to -2 has no fixed result, so those cases are dropped",
    "isdigit": "isdigit of an int from -1 to 255, the raw result: 4 for a decimal digit, else 0; an int from -128 to -2 has no fixed result, so those cases are dropped",
    "isalnum": "isalnum of an int from -1 to 255, the raw result: 4 for a decimal digit, 1 for an upper case letter, 2 for a lower case one, else 0; an int from -128 to -2 has no fixed result, so those cases are dropped",
    "strlwr": "_strlwr of a string: the string after it",
    "strncpy": "strncpy of a source string and a count into a 24 byte destination filled with 0xAA beforehand: all 24 bytes after it, so the zero fill shows",
    "strstr": "strstr of a string and a needle: the offset of the match, or -1",
    "strchr": "strchr of a string and a character (0 included): the offset of the match, or -1",
    "strrchr": "strrchr of a string and a character (0 included): the offset of the match, or -1",
    "strcspn": "strcspn of a string and a set of characters, either possibly empty: the length of the prefix without them",
    "strtok": "strtok over a string in a 48 byte buffer filled with 0xAA beforehand, the first call with `delims`, the next calls with `delims2` until it returns null: the token count, `toks` as 24 pairs of i32 (offset in the buffer, length) of which the count's worth are used, then two more pairs and zeros to the end of `toks`, and the buffer afterward. The first further pair is (-1, 0): -1 for the null that ended the calls (-2 would stand there had the calls stopped at 22 tokens, which no record reaches). The second is (r, 0), r being what one more call with `delims2` returns, -1 for a null and otherwise its offset; every record holds -1 there, since a call after the null returns null",
    "memmove": "memmove within a 64 byte buffer holding byte i = (37 * i + 11) mod 256: the offset of the returned pointer and the buffer afterward, for overlapping and disjoint ranges",
    "sprintf": "sprintf of a format with arguments: `kinds` has one letter per conversion argument in order, and `args` holds the arguments in that order, an i (an int) as 4 bytes, an f (a double) as 8 and an s (a string) as a u32 length and its bytes; the outputs are the text written and the return value (Rules)",
    "sscanf": "sscanf of a text with a format whose conversions are integers, words and sets (none is `%f`): the return value, the four ints stored (0x5a5a5a5a where nothing was stored) and the 64 bytes of the string buffer (0xAA where nothing was stored) (Rules)",
    "ftol": "the conversion of a double to an int that game logic uses: the double truncated toward zero to a 64-bit integer, of which the result is the low 32 bits; 0 for a NaN, an infinity and a magnitude of 2^63 or more",
}

RULES = """
## Rules

Each rule is what the original's runtime does, stated so that a runtime can be written from it; every record of the
entries it names follows it. Rounding "to nearest" breaks a tie to the even neighbour unless the rule says otherwise.

### Sine table

`sine`, `cosine`, `scaled_sine`, `scaled_cosine` and `rotation_matrix` read one table of 640 signed entries, entry i
being round(sin(i * 2 * pi / 512) * 8192): the sine table of `runtime/bitexact_tables.py`. For an angle a (0 to 65535),
the sine is entry `(a >> 7) & 511` and the cosine entry `128 + ((a >> 7) & 511)`.

### Scaled sine and cosine

The entry is `((a + 32) >> 7) & 511` for `scaled_sine` and `((a + 0x4020) >> 7) & 511` for `scaled_cosine`. The result
is `(entry * scale + 4096) >> 13`, computed in 64 bits, of which it is the low 32 bits, so a large scale wraps.

### Rotation matrix

Let ca and sa be the cosine and sine of `angle_a`, cb and sb those of `angle_b`, and cc and sc those of `angle_c`, by
the sine table. Write `p * q` for the 64-bit product of p and q shifted right 13 and kept as its low 32 bits, taken
left to right (`p * q * r` is `(p * q) * r`). Every sum, difference and negation wraps at 32 bits. The entries, in
the record's order, are

    m0 = cb * cc
    m1 = sc * ca + sb * sa * cc
    m2 = sa * sc - ca * sb * cc
    m3 = -(cb * sc)
    m4 = ca * cc - sa * sb * sc
    m5 = ca * sb * sc + sa * cc
    m6 = sb
    m7 = -(sa * cb)
    m8 = ca * cb

### Applying a matrix

`apply_rotation` multiplies by the transpose, each product shifted on its own (the `*` of "Rotation matrix") and the
sums wrapping:

    rx = x * m0 + y * m3 + z * m6
    ry = x * m1 + y * m4 + z * m7
    rz = x * m2 + y * m5 + z * m8

When `aliased` is 1, rx replaces x before ry is computed and ry replaces y before rz is, so ry reads the new x, and rz
the new x and y.

### Rotating a point

An angle of 0 gives (x, z) unchanged. Otherwise let t be the angle as a signed 16-bit number (65535 is -1), and theta
the product, rounded to the nearest double, of t and the double nearest 9.587379924285e-05. S and C are the sine and
cosine of theta rounded to nearest with a 64-bit mantissa: the 80-bit sine and cosine table of
`runtime/bitexact_tables.py`. Each of x * C, z * C, x * S and z * S is the exact product rounded to the nearest double.
Then rx is x * C - z * S and rz is z * C + x * S, each a double, rounded to the nearest int; a result outside the int
range is -2147483648.

### Angle of a vector

The vector (0, 0) gives 0. Otherwise the result is atan2(|x|, z), in [0, pi], times the double nearest 10430.37835047,
rounded to the nearest int, and negated when x is negative. No record lies within 10^-9 of a tie.

### hypot

An infinite argument gives +infinity; otherwise a NaN argument is returned as it is (x before y), and two zeros give 0.
Otherwise, with big the larger of |x| and |y|, each step is rounded to nearest with a 64-bit mantissa and then to a
double, except where it says otherwise:

1. ra = |x| / big and rb = |y| / big;
2. the sum of the squares ra * ra and rb * rb: each square is rounded to a 64-bit mantissa only, and the sum to a 64-bit
   mantissa and then to a double;
3. the square root of that sum;
4. that root times big.

### acos

x outside [-1, 1] gives a NaN, and a NaN is returned as it is. Otherwise s is sqrt((1 + x) * (1 - x)), each of the
sum, the difference, the product and the square root a double rounded to nearest. The result is the angle whose
cosine and sine are in the ratio x to s: the arctangent of s over x for x of 0 or more, and pi minus the arctangent of
s over -x for a negative x. It is rounded to nearest with a 64-bit mantissa and then to a double.

### Text to a number

`float_text` and `double_text` share one scanner, one multiplication and one rounding. For `double_text` the text
first loses its leading bytes 9 to 13 and 32.

1. Leading spaces, tabs, line feeds and carriage returns are skipped, then one `+` or `-`. A text that then has neither
   a digit nor a `.` followed by a digit gives +0.
2. Leading zeros are skipped, and the digits of the integer part and then the fraction part are read. The first 25
   significant digits are kept. An integer digit after them adds one to the power of ten and a fraction digit after
   them is ignored; before the first kept digit, each zero of the fraction takes one from the power, and each kept
   fraction digit takes one from it too.
3. An exponent is `e`, `E`, `d` or `D`, an optional sign and at least one digit; without a digit there is no exponent.
   Its value stops growing once it passes 5200, and is then 5201.
4. When 25 digits are kept, the 24th is raised by one if it is 5 or more (it may become 10), the 25th is dropped and the
   power gains one. Then each trailing zero digit is dropped and adds one to the power. No digit left gives a zero
   with the text's sign.
5. The kept digits make an integer D, the last digit counting 10 when step 4 made it 10, held exactly as an 80-bit
   mantissa. The power P is the exponent, negated after a `-`, plus the adjustments. A P above 5200 gives an infinity
   and one below -5200 a zero, with the text's sign.
6. D is then multiplied in turn by one factor for each nonzero base-8 digit of |P|, the lowest first: the digit d (1
   to 7) at the place g (0 to 4) gives the factor 10^(d * 8^g), or 10^-(d * 8^g) when P is negative. A factor is that
   power rounded to nearest with an 80-bit mantissa: the powers-of-ten tables of `runtime/bitexact_tables.py`, which
   store an entry whose low 16 bits are 0x8000 or more one unit of its top 64 bits too high, so a reader takes that
   unit off. An infinite product gives an infinity and a zero product a zero, with the text's sign.
7. The 80-bit result is rounded to the target, a float (24 bits) or a double (53 bits), as "Rounding to the target"
   says, and given the text's sign.

**The multiplication.** Write each 80-bit mantissa as five 16-bit words, x0 (the lowest) to x4 and y0 to y4, and each
exponent as biased by 16383 with the mantissa's top bit worth 2^0, ex and ey.

1. When ex or ey is 32767, or ex + ey is above 49149, the product is infinite; when ex + ey is 16319 or less, it is 0.
2. An operand whose exponent is 0 adds one to ex + ey, and makes the product 0 when its mantissa is 0.
3. Six 16-bit words a0 to a5 start at 0. For each i from 0 to 4 and each j from 0 to 4 - i, the 32-bit product of
   x(i + j) and y(4 - j) is added to the 32-bit number whose low word is a(i) and high word a(i + 1). A carry out of
   those 32 bits adds one to a(i + 2) alone, which wraps at 16 bits and carries no further.
4. The exponent is ex + ey - 16382. While it is above 0 and the top bit of a5 is clear, the 96 bits a5 to a0 shift
   left one place and the exponent drops by one. If the exponent is then 0 or less, it drops by one more, and while it
   is below 0 the 96 bits shift right one place and it rises by one; a 1 shifted out of a0 by these right shifts sets
   the lowest bit of a0 once they end.
5. The mantissa is a5 to a1, rounded by a0: up by one unit when a0 is above 0x8000, or is 0x8000 and a1 is odd. A carry
   out of the top makes the mantissa its top bit alone and adds one to the exponent. An exponent of 32767 or more is an
   infinity.

**Rounding to n bits** (rule R) keeps the top n bits of a mantissa and adds one unit to the last kept bit exactly when
the first dropped bit is 1 and a dropped bit from the third on is 1. The second dropped bit is not looked at, so a
remainder of exactly one half or exactly three quarters of a unit rounds down, and any other remainder above one half
rounds up.

**Rounding to the target.** Let e be the exponent of the 80-bit result (its value is 1.f * 2^e) and n the target's
bits, 24 or 53. An 80-bit result with a biased exponent of 0 is a zero. Otherwise round the mantissa to n bits by rule
R; a carry out of the top adds one to e. Then, with the float's bounds first and the double's in parentheses:

- e of 128 (1024) or more: an infinity;
- e from -126 to 127 (-1022 to 1023): the normal number with that exponent and the rounded mantissa;
- e below -151 (-1076): a zero;
- e from -151 to -127 (-1076 to -1023): a denormal. Go back to the mantissa before the rounding, held as a 96-bit
  word (its 80 bits, then 16 zero bits), and shift the word right by -127 - e places (-1023 - e), with the e that the
  rounding gave: zeros enter at the top, and a bit shifted out of the bottom is lost. Round the shifted word to n bits
  by rule R, dropping any carry out of the top. The n bits are the word's top n bits, the zeros the shift brought in
  included (not n bits counted from its first 1), the dropped bits are the rest of the word, and a lost bit counts for
  nothing: no sticky bit stands for the bits shifted out. The denormal's fraction field is the top n - 1 of those n
  bits; the last one is dropped.

So when the first rounding does not carry, a denormal is the multiple of the smallest denormal below the value, plus
one more when the remainder is above three quarters of the smallest denormal and is not exactly seven eighths of it: a
remainder of exactly 0.75 of it rounds down.

### sprintf

The formats of the records use these conversions alone:

- `%d`, `%i` and `%ld`: a signed decimal. `%+d` writes a sign, `+0` for 0. A width (`%3d`, `%4d`) pads with spaces on
  the left, and a `0` before it (`%02d`, `%04i`) with zeros after the sign; the sign counts in the width, so -1 at
  `%02d` is `-1`.
- `%x`, `%lx` and `%lX`, with a width and `0` padding (`%02x`, `%04x`, `%08x`, `%08lx`, `%04lX`, `%08lX`): the int's
  32 bits as an unsigned number in hex, with lower or upper case digits.
- `%c`: the int's low byte, written and counted even when it is 0.
- `%s`: the string's bytes as they are.
- `%f` with the precision 0 or 1 (`%.0f`, `%.1f`, `%1.1f`), below.
- `%%`: one `%`.

The return value is the number of bytes written, without the closing 0.

`%f` of a finite double: the double's exact value is rounded to 17 significant digits, a tie away from zero, and
every digit after those is 0. That decimal is then rounded to the precision, a tie away from zero (0.25 at precision 1
is `0.3`, 0.5 at precision 0 is `1`). A negative value that rounds to zero keeps its `-` (-6.5e-06 at precision 1 is
`-0.0`), but a negative zero is written `0.0`, or `0` at precision 0. Records do print infinities and NaNs (all of
them quiet): `1` at precision 0 and `1.$` at precision 1, after a `-` when the argument's sign bit is set.

### sscanf

The formats of the records use `%d`, `%i`, `%x`, `%s`, `%[...]` (a set of ranges and characters) and `%n`, none with a
width, a `*` or a size letter, and white space and `,` between them.

- A white-space directive skips the bytes 9 to 13 and 32 (not 0xA0 or any other byte above 127), and so do `%d`, `%i`,
  `%x` and `%s` before they read; `%[...]` and `%n` do not.
- `%d` reads an optional sign and decimal digits; `%x` an optional sign, an optional `0x` or `0X` and hex digits; `%i`
  reads hex after a `0x` or `0X`, octal after any other leading 0, and decimal otherwise. The value is accumulated
  modulo 2^32 and negated after a `-`: 2147483648 at `%d` stores -2147483648, and 4294967296 stores 0. A first byte
  that cannot start the number stores nothing and ends the scan, and so does a sign or a `0x` followed by a byte that
  is not a digit of the number (`-ff` at `%d`, `0xg` at `%x`). A digit is `0` to `9`, and for hex also `a` to `f` and
  `A` to `F`; the number ends at the first byte that is not one, a byte above 127 included.
- `%s` stores the bytes up to the next white space and a 0, so a byte above 127 belongs to the word; `%[...]` stores
  the bytes in the set, at least one, and a 0. A range `a-z` in a set holds the bytes from `a` to `z` by their
  unsigned value, and no byte above 127 is in the sets of the records.
- Where each value goes: every format has at most one `%s` or `%[...]`, and it writes its bytes and the 0 at the start
  of the string buffer, leaving the rest of the buffer 0xAA. The four ints take the integer conversions in order: each
  `%d`, `%i`, `%x` and `%n` that stores takes the next int, the first one first, so `%n` takes an int too
  (` %s %n` stores its count in the first int, and `%[a-zA-Z],%i,%i,%i` its three numbers in the first three). The
  string conversion takes no int. A conversion that stores nothing ends the scan, so no int is skipped, and every int
  after the last one stored stays 0x5a5a5a5a.
- The return value counts the values stored, `%n` not among them. It is -1 instead when the input ends before the
  first value is stored: an empty text, white space alone before a conversion that skips it, or a text that ends in a
  sign or a `0x` (`-` at `%d`, `0x` at `%x`).
- `%n` stores the number of bytes read so far, except when the conversion before it ended at the end of the input:
  ` %s %n` stores 3 on `12 34` and 9 on `   abc   ` (the input ends in the white space, and `%n` still stores), but
  nothing on `0`, where `%s` read up to the end of the input.

### itoa

`_itoa` writes the value's digits, the most significant first, with no leading zeros (0 is `0`) and a 0 after them.
The digits above 9 are the lower case letters, `a` for 10 to `f` for 15. In radix 10 a negative value is written as
`-` and the digits of its magnitude (-2147483648 is `-2147483648`). In every other radix the value's 32 bits are
written as an unsigned number, with no sign: -1 is `ffffffff` in radix 16, `37777777777` in radix 8 and thirty-two
`1`s in radix 2. The records use the radices 2, 8, 10 and 16.

### Bytes above 127

Every string and character entry reads a byte as unsigned, 0 to 255, and the C runtime's classes hold ASCII alone: the
letters are `A` to `Z` (upper case) and `a` to `z` (lower case), the digits `0` to `9`, and the white space the bytes 9
to 13 and 32. A byte from 128 to 255 is none of these. A string ends at its first 0 byte; every byte from 1 to 255
belongs to it.

- `isalnum`, `isdigit` and `isspace`: an int from 128 to 255 gives 0, as -1 does.
- `tolower` and `toupper` change only `A` to `Z` (to 97 to 122) and `a` to `z` (to 65 to 90): every other int from
  -128 to 255 is returned as it is, so 0xC9 and 0xE9 are not a case pair and -23 stays -23.
- `_strlwr` changes only the bytes `A` to `Z`; every other byte, those above 127 included, stays.
- `strncmp` compares the bytes as unsigned numbers, so a byte above 127 sorts after every ASCII byte, and the end of
  the shorter string is a 0 that sorts before every byte. The result is the sign of the first difference (-1, 0 or 1),
  and 0 when the strings agree up to the count or up to a 0 they share.
- `_strcmpi` and `_strnicmp` do the same after turning each `A` to `Z` into its lower case letter, and nothing else:
  `_` (95) sorts before `A`, which compares as `a` (97), and a byte above 127 still sorts after every ASCII byte.
- `strchr` and `strrchr` look for the int's low byte, as unsigned: -23 finds 0xE9, and 0 or 256 finds the string's
  closing 0, at the offset of the string's length. `strcspn`, `strstr` and `strtok` match bytes by exact value, and a
  byte above 127 in a set or a needle matches only itself.
- `atoi` skips only the white space above (not 0xA0), then one `+` or `-`, then reads digits `0` to `9` until any other
  byte, the value accumulated modulo 2^32 and negated after a `-`; no digit gives 0.
- `%s` and `%d`, `%i` and `%x` of `sscanf` treat these bytes as "sscanf" says: part of a word, and not a digit.
"""

HEADER = """# Runtime vectors

What the original program's runtime returns for the calls game logic makes, as inputs and results, so that a runtime
written from the behaviour alone can be held to them. Every result here was recorded from the original's own code
for exactly the inputs beside it. `vectors.json` lists the entries; `layouts.txt` gives each entry's field types, one
line per entry, `name inputs outputs` with the types comma separated.

## Records

`<entry>.bin` is a run of records, one per case, little endian: the record's inputs in the order of the manifest's
`inputs`, then its outputs in the order of `outputs`. A field has one of these types:

| type | bytes |
| --- | --- |
| `i32` | 4, a signed integer |
| `u32` | 4, an unsigned integer |
| `f64` | 8, an IEEE double (its bytes, low word first) |
| `str` | a u32 length, then that many bytes (text, or bytes: no terminator is stored, and a byte may be 0x80 to 0xff) |
| `bytes<N>` | exactly N bytes (`bytes16` is 16 bytes) |

A float is stored as its 32-bit pattern in a `u32`. A record that holds a `str` has no fixed size, so a reader walks
the file from its start.

## Edge and random records

A file holds `edge` records first and `random` records after them. The edge records are the named special inputs and
the exhaustive small domains; the random records come from a fixed generator. `edge_total` and `random_total` count
the cases the original program was run on, and the file keeps a part of them when that is fewer. `dropped` counts the
cases left out because the result is not one a runtime can be held to (it is 7 for `acos`, whose recorded result there
differs from the rule of "Rules", and 127 for each of the three character classes, which have no fixed result below -1).

## Sampling

Every entry keeps every edge record. The random records of every entry are sampled to 80000 bytes, or kept whole when
they are fewer. A sample of m of n
records takes one record from each of m strata, the stratum j being the indexes from floor(j * n / m) up to (not
including) floor((j + 1) * n / m), and in it the index offset `mix(j) mod size` from the stratum's start, with `mix` the
64-bit mixing function

    z = (j + 0x9E3779B97F4A7C15) mod 2^64
    z = ((z xor (z >> 30)) * 0xBF58476D1CE4E5B9) mod 2^64
    z = ((z xor (z >> 27)) * 0x94D049BB133111EB) mod 2^64
    mix(j) = z xor (z >> 31)

and m is the sample's byte cap divided by the entries' mean record size (at least 1). The sampled records stay in
their original order.

## Entries
"""

FOOTER = """
## Left out

- `qsort`: game logic has no qsort call, and the order in which equal elements end is left open.
"""


def mix(x: int) -> int:
    """The 64-bit mixing function of the README."""
    z = (x + 0x9E3779B97F4A7C15) & MASK
    z = ((z ^ (z >> 30)) * 0xBF58476D1CE4E5B9) & MASK
    z = ((z ^ (z >> 27)) * 0x94D049BB133111EB) & MASK
    return z ^ (z >> 31)


def strata(total: int, m: int) -> list[int]:
    """The m indexes of range(total), one from each of m equal strata, by `mix`."""
    picked = []
    for j in range(m):
        lo = j * total // m
        hi = (j + 1) * total // m
        picked.append(lo + mix(j) % (hi - lo))
    return picked


def type_size(name: str) -> int | None:
    """A type's size in bytes, None for `str`, and ValueError for a name that is no type."""
    if name in FIXED:
        return FIXED[name]
    if name == "str":
        return None
    m = BYTES.fullmatch(name)
    if m:
        return int(m.group(1))
    raise ValueError(f"unknown type {name!r}")


def fixed_size(types: list[str]) -> int | None:
    """The size of a record of these types, None when it holds a str."""
    sizes = [type_size(t) for t in types]
    return None if None in sizes else sum(sizes)


def walk(data, types: list[str], start: int = 0):
    """Yield (begin, end) of each record of data from start; ValueError when a record runs past the end."""
    sizes = [type_size(t) for t in types]
    size = len(data)
    pos = start
    while pos < size:
        begin = pos
        for s in sizes:
            if s is None:
                if pos + 4 > size:
                    raise ValueError(f"record at {begin} runs past the end")
                pos += 4 + struct.unpack_from("<I", data, pos)[0]
            else:
                pos += s
            if pos > size:
                raise ValueError(f"record at {begin} runs past the end")
        yield begin, pos


def count_records(data, types: list[str]) -> int:
    """The number of records in data; ValueError when it does not end on a record."""
    fixed = fixed_size(types)
    if fixed is not None:
        if len(data) % fixed:
            raise ValueError(f"{len(data)} bytes is not a multiple of {fixed}")
        return len(data) // fixed
    return sum(1 for _ in walk(data, types))


def parse_layout(text: str) -> tuple[list[list[str]], list[list[str]]]:
    """The (inputs, outputs) of a layout file: `in name type` and `out name type` lines."""
    inputs: list[list[str]] = []
    outputs: list[list[str]] = []
    for line in text.splitlines():
        if not line.strip():
            continue
        kind, name, type_name = line.split()
        type_size(type_name)
        {"in": inputs, "out": outputs}[kind].append([name, type_name])
    return inputs, outputs


def layout_line(name: str, entry: dict) -> str:
    """The layouts.txt line of an entry."""
    return f"{name} {','.join(t for _, t in entry['inputs'])} {','.join(t for _, t in entry['outputs'])}"


def open_data(path: Path):
    """The bytes of a file, memory mapped; an empty file or a missing one is b''."""
    if not path.exists() or path.stat().st_size == 0:
        return b""
    with open(path, "rb") as f:
        return mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ)


def pick(data, types: list[str], total: int, indexes: list[int], first: int = 0) -> list[bytes]:
    """The records numbered by indexes (sorted, counted from `first`) of data."""
    fixed = fixed_size(types)
    if fixed is not None:
        return [bytes(data[i * fixed:(i + 1) * fixed]) for i in indexes]
    wanted = iter(indexes)
    want = next(wanted, None)
    kept = []
    for n, (begin, end) in enumerate(walk(data, types)):
        if want is None:
            break
        if n == want:
            kept.append(bytes(data[begin:end]))
            want = next(wanted, None)
    return kept


def sample(data, types: list[str], cap: int) -> list[bytes]:
    """Every record when data fits cap bytes, else the sample of mean-size records that does."""
    total = count_records(data, types)
    if total == 0:
        return []
    if len(data) <= cap:
        return pick(data, types, total, list(range(total)))
    m = max(1, min(total, cap * total // len(data)))
    return pick(data, types, total, strata(total, m))


def edge_records(data, types: list[str]) -> list[bytes]:
    """Every edge record of data."""
    if len(data) == 0:
        return []
    return pick(data, types, 0, list(range(count_records(data, types))))


def memory_dependent(record: bytes) -> bool:
    """True for a character class record of an int below -1."""
    return struct.unpack_from("<i", record, 0)[0] < -1


def build(scratch: Path, out: Path) -> dict:
    """Write the directory `out` from the kit files of `scratch`; the manifest."""
    entries = {}
    files = {}
    for layout in sorted(scratch.glob("*.layout.txt")):
        name = layout.name[: -len(".layout.txt")]
        inputs, outputs = parse_layout(layout.read_text())
        types = [t for _, t in inputs + outputs]
        edge_data = open_data(scratch / f"{name}.edge.bin")
        random_data = open_data(scratch / f"{name}.random.bin")
        edge = edge_records(edge_data, types)
        edge_total = len(edge)
        random_total = count_records(random_data, types) if len(random_data) else 0
        random = sample(random_data, types, RANDOM_CAP)
        dropped_file = scratch / f"{name}.dropped.txt"
        dropped = int(dropped_file.read_text()) if dropped_file.exists() else 0
        if name in MEMORY_DEPENDENT:
            kept = [r for r in edge if not memory_dependent(r)]
            dropped += len(edge) - len(kept)
            edge_total -= len(edge) - len(kept)
            edge = kept
        files[f"{name}.bin"] = b"".join(edge) + b"".join(random)
        entries[name] = {"file": f"{name}.bin", "inputs": inputs, "outputs": outputs, "edge": len(edge),
                         "edge_total": edge_total, "random": len(random), "random_total": random_total,
                         "dropped": dropped}
    manifest = {"format": 1, "entries": entries, "omitted": dict(OMITTED)}
    out.mkdir(parents=True, exist_ok=True)
    for stale in out.iterdir():
        if stale.is_file():
            stale.unlink()
    for file, data in files.items():
        (out / file).write_bytes(data)
    (out / MANIFEST).write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n")
    (out / LAYOUTS).write_text("".join(layout_line(n, entries[n]) + "\n" for n in sorted(entries)))
    (out / README).write_text(readme(entries))
    return manifest


def readme(entries: dict) -> str:
    """The README text for these entries."""
    lines = [HEADER]
    first = [n for n in LIBRARY if n in entries]
    rest = [n for n in sorted(entries) if n not in first]
    for n in first + rest:
        e = entries[n]
        text = DESCRIPTIONS.get(n, "")
        lines.append(f"- `{n}` ({e['edge']} edge, {e['random']} random of {e['random_total']}): {text}")
    lines.append(RULES)
    lines.append(FOOTER)
    return "\n".join(lines)


def kit_run(scratch: Path) -> int:
    """Run tools/bitexact.py --kit into a clean scratch directory."""
    scratch.mkdir(parents=True, exist_ok=True)
    for old in scratch.iterdir():
        if old.is_file() and old.suffix in (".bin", ".txt"):
            old.unlink()
    env = dict(os.environ, WINEARCH=os.environ.get("WINEARCH", "win64"))
    return subprocess.run([sys.executable, str(ROOT / "tools/bitexact.py"), "--kit", str(scratch)], env=env,
                          cwd=ROOT).returncode


def check(directory: Path) -> list[str]:
    """One line per problem of a vectors directory."""
    problems: list[str] = []
    manifest_path = directory / MANIFEST
    try:
        manifest = json.loads(manifest_path.read_text())
    except (OSError, ValueError) as e:
        return [f"{manifest_path}: {e}"]
    if not isinstance(manifest, dict) or manifest.get("format") != 1:
        return [f"{MANIFEST}: format is not 1"]
    entries = manifest.get("entries")
    omitted = manifest.get("omitted")
    if not isinstance(entries, dict) or not isinstance(omitted, dict):
        return [f"{MANIFEST}: entries and omitted must be objects"]
    expected = {MANIFEST, LAYOUTS, README}
    total = 0
    layouts = []
    for name in sorted(entries):
        entry = entries[name]
        try:
            file = entry["file"]
            inputs, outputs = entry["inputs"], entry["outputs"]
            counts = {k: entry[k] for k in ("edge", "random", "random_total", "dropped")}
            types = [t for _, t in inputs + outputs]
            for t in types:
                type_size(t)
        except (KeyError, TypeError, ValueError) as e:
            problems.append(f"{name}: malformed manifest entry ({e!r})")
            continue
        if any(not isinstance(v, int) or v < 0 for v in counts.values()):
            problems.append(f"{name}: a count is not a non-negative integer")
            continue
        if counts["random"] > counts["random_total"]:
            problems.append(f"{name}: random {counts['random']} is more than random_total {counts['random_total']}")
        edge_total = entry.get("edge_total", counts["edge"])
        if not isinstance(edge_total, int) or edge_total < counts["edge"]:
            problems.append(f"{name}: edge_total is below edge")
        if Path(file).name != file or file != f"{name}.bin":
            problems.append(f"{name}: file {file!r} is not {name}.bin")
        expected.add(file)
        layouts.append(layout_line(name, entry))
        path = directory / file
        if not path.is_file():
            problems.append(f"{name}: {file} is missing")
            continue
        size = path.stat().st_size
        total += size
        data = open_data(path)
        try:
            count = count_records(data, types)
        except (ValueError, struct.error) as e:
            problems.append(f"{name}: {file}: {e}")
            continue
        if count != counts["edge"] + counts["random"]:
            problems.append(f"{name}: {file} holds {count} records, the manifest says "
                            f"{counts['edge']} edge and {counts['random']} random")
    for name, reason in sorted(omitted.items()):
        if not isinstance(reason, str) or not reason:
            problems.append(f"omitted {name}: no reason")
        if name in entries:
            problems.append(f"{name} is both an entry and omitted")
    for path in sorted(directory.iterdir()):
        if path.name not in expected:
            problems.append(f"{path.name}: not in the manifest")
    for extra in (LAYOUTS, README):
        if not (directory / extra).is_file():
            problems.append(f"{extra} is missing")
    if (directory / LAYOUTS).is_file() and (directory / LAYOUTS).read_text() != "".join(l + "\n" for l in layouts):
        problems.append(f"{LAYOUTS} does not match the manifest")
    if (directory / README).is_file():
        text = (directory / README).read_text()
        for name in sorted(list(entries) + list(omitted)):
            if not re.search(rf"(?<![A-Za-z0-9_]){re.escape(name)}(?![A-Za-z0-9_])", text):
                problems.append(f"{README} does not name {name}")
    size_all = sum(p.stat().st_size for p in directory.iterdir() if p.is_file())
    if size_all > BUDGET:
        problems.append(f"{directory}: {size_all} bytes is over the budget of {BUDGET}")
    return problems


def readme_problems(directory: Path) -> list[str]:
    """A problem when README.md is not the text `readme` gives for the manifest's entries."""
    entries = json.loads((directory / MANIFEST).read_text())["entries"]
    if (directory / README).read_text() != readme(entries):
        return [f"{README} is not readme(entries): run uv run tools/runtimevectors.py readme"]
    return []


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    sub = parser.add_subparsers(dest="command", required=True)
    write = sub.add_parser("write", help="run the kit and write tests/runtime/vectors/")
    write.add_argument("--scratch", type=Path, default=SCRATCH, help=f"where the kit run writes (default {SCRATCH})")
    write.add_argument("--reuse", action="store_true", help="use the kit files already in --scratch")
    write.add_argument("--out", type=Path, default=OUT)
    verify = sub.add_parser("check", help="check the committed directory")
    verify.add_argument("--dir", type=Path, default=OUT)
    rewrite = sub.add_parser("readme", help="rewrite README.md from vectors.json")
    rewrite.add_argument("--dir", type=Path, default=OUT)
    args = parser.parse_args(argv)
    if args.command == "readme":
        entries = json.loads((args.dir / MANIFEST).read_text())["entries"]
        (args.dir / README).write_text(readme(entries))
        return 0
    if args.command == "write":
        scratch = args.scratch.resolve()
        if not args.reuse:
            code = kit_run(scratch)
            if code != 0:
                print(f"runtimevectors: tools/bitexact.py --kit failed (exit {code})", file=sys.stderr)
                return 1
        manifest = build(scratch, args.out)
        size = sum(p.stat().st_size for p in args.out.iterdir())
        print(f"runtimevectors: {len(manifest['entries'])} entries, {size} bytes in {args.out}")
        problems = check(args.out)
        for p in problems:
            print(p, file=sys.stderr)
        return 1 if problems else 0
    problems = check(args.dir) or readme_problems(args.dir)
    for p in problems:
        print(p, file=sys.stderr)
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
