# Runtime vectors

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
cases left out because the result is not one a runtime can be held to (it is 7 for `acos`, where the processor's
arctangent is not always the correctly rounded value, and 127 for each of the three character classes, whose results below -1
depend on memory).

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

- `sine` (65536 edge, 0 random of 0): the sine of a 16-bit angle (65536 is a full turn) as a fixed-point int, 8192 being 1.0; every angle is an edge record
- `cosine` (65536 edge, 0 random of 0): the cosine of a 16-bit angle in the same fixed point; every angle is an edge record
- `mul_hi` (16129 edge, 6666 random of 1000000): the high 32 bits of the signed 64-bit product of two ints
- `scaled_sine` (1016 edge, 6666 random of 9846344): scale times the sine of an angle, the angle rounded to a table step and the product rounded to nearest, as an int
- `scaled_cosine` (1016 edge, 6666 random of 9846344): scale times the cosine of an angle, in the same rounding
- `angle_of_vector` (16129 edge, 6666 random of 3361201): the angle (32768 is half a turn) of the vector (z, x), the arctangent of x over z, rounded to nearest even
- `rotate_point` (1152 edge, 4000 random of 12533184): the point (x, z) rotated by an angle (rx, rz); an angle of 0 leaves it, a result outside the int range is -2147483648
- `rotation_matrix` (1536 edge, 1666 random of 13581376): the 3x3 rotation matrix (m0 to m8, row by row) built from three angles
- `apply_rotation` (0 edge, 1250 random of 1250000): a vector (x, y, z) times a 3x3 matrix (m0 to m8, row by row), giving (rx, ry, rz); `aliased` is 1 when the result overwrites the vector it is computed from
- `mul_div` (294 edge, 5000 random of 2476073): a times b divided by c, the product taken in 64 bits and the quotient truncated toward zero, 0 when c is 0; a case whose quotient does not fit an int is not recorded
- `sqrt` (0 edge, 5000 random of 2000000): the square root of a non-negative double
- `sqrt_float` (0 edge, 10000 random of 2000000): the square root of a non-negative float, given and returned as its 32-bit pattern
- `hypot` (0 edge, 3333 random of 2000000): the length of the vector (x, y) by the C runtime's hypot, finite arguments: big is the larger magnitude, the result is the square root of (x / big)^2 + (y / big)^2, times big, each step rounded to a 64-bit mantissa and stored as a double
- `acos` (5 edge, 5000 random of 499988): the arccosine of x in [-1, 1] as a double; a case the processor's arctangent rounds differently from the library's own result is dropped
- `float_text` (0 edge, 3742 random of 2000000): the float the C runtime's scanf conversion `%f` stores for a text, as the float's 32-bit pattern; it is not always the correctly rounded float
- `double_text` (131 edge, 3072 random of 1100000): the double the C runtime's atof returns for a text: leading white space skipped, any trailing text ignored, overflow to infinity, underflow to zero or a denormal
- `rand` (8 edge, 1 random of 1492): the first 10000 values of the C runtime's rand after srand(seed), as 10000 little endian i32 (40000 bytes)
- `shuffle` (526 edge, 25 random of 6094): the ints 1, 4, 7, ... (3 * index + 1) for size elements after the C++ library's shuffle of a range, drawing from rand seeded by `seed`, as `size` little endian i32 behind a u32 byte count, then the next four values of rand
- `load_sequence` (64 edge, 1250 random of 2336): the load sequence: srand(seed), the 391606 rand draws the explosion setup makes, then the same shuffle of the first `size` (3 to 10) of the ints 0 to 9, as 10 little endian i32, then the next four values of rand
- `atoi` (2460 edge, 4243 random of 6000): atoi of a string: white space of every kind, signs, leading zeros, trailing text, values around and past the int range and 2^32, high bytes
- `compare_bytes` (256 edge, 0 random of 0): every pair of strings of at most one byte: for the first string's byte `a` (0 to 255, 0 being the empty string), the results of _strcmpi, of _strnicmp with the counts 1 and 2 and of strncmp with the counts 1 and 2 against the second string of each byte b from 0 to 255, each output holding one signed byte (-1, 0 or 1) per b in order
- `ftol` (134 edge, 6666 random of 8000): the conversion of a double to an int by the compiled code of the original program (a call to its _ftol); the result is the one the original gave, including for values out of range and for NaN
- `isalnum` (257 edge, 0 random of 0): isalnum of an int from -1 to 255, the raw result; the results for -128 to -2 are dropped
- `isdigit` (257 edge, 0 random of 0): isdigit of an int from -1 to 255, the raw result; the results for -128 to -2 are dropped
- `isspace` (257 edge, 0 random of 0): isspace of an int from -1 to 255, the raw result; the results for -128 to -2 depend on memory before the library's table and are dropped
- `itoa` (755 edge, 4621 random of 8000): _itoa of a value in a radix: the text it writes
- `memmove` (1818 edge, 1000 random of 12000): memmove within a 64 byte buffer holding byte i = (37 * i + 11) mod 256: the offset of the returned pointer and the buffer afterward, for overlapping and disjoint ranges
- `sprintf` (5682 edge, 1018 random of 7220): sprintf of a format with arguments: `kinds` has one letter per conversion argument in order, and `args` holds the arguments in that order, an i (an int) as 4 bytes, an f (a double) as 8 and an s (a string) as a u32 length and its bytes; the outputs are the text written and the return value
- `sscanf` (549 edge, 745 random of 5400): sscanf of a text with a format whose conversions are integers, words and sets (none is `%f`): the return value, the four ints stored (0x5a5a5a5a where nothing was stored) and the 64 bytes of the string buffer (0xAA where nothing was stored)
- `strchr` (1452 edge, 4455 random of 10000): strchr of a string and a character (0 included): the offset of the match, or -1
- `strcmpi` (3136 edge, 3317 random of 20000): _strcmpi of two strings: the result (-1, 0 or 1)
- `strcspn` (4840 edge, 4036 random of 10000): strcspn of a string and a set of characters, either possibly empty: the length of the prefix without them
- `strlwr` (259 edge, 2808 random of 6000): _strlwr of a string: the string after it
- `strncmp` (16981 edge, 2845 random of 20000): strncmp of two strings and a count: the result (-1, 0 or 1)
- `strncpy` (525 edge, 1909 random of 4000): strncpy of a source string and a count into a 24 byte destination filled with 0xAA beforehand: all 24 bytes after it, so the zero fill shows
- `strnicmp` (16981 edge, 2845 random of 20000): _strnicmp of two strings and a count: the result (-1, 0 or 1)
- `strrchr` (1452 edge, 4455 random of 10000): strrchr of a string and a character (0 included): the offset of the match, or -1
- `strstr` (4840 edge, 4036 random of 10000): strstr of a string and a needle: the offset of the match, or -1
- `strtok` (540 edge, 286 random of 6000): strtok over a string in a 48 byte buffer filled with 0xAA beforehand, the first call with `delims`, the next calls with `delims2` until it returns null: the token count, `toks` as 24 pairs of i32 (offset in the buffer, length) of which the count's worth are used, then -1 (a null) or -2 and the offset a further call returns, and the buffer afterward
- `tolower` (384 edge, 0 random of 0): tolower of an int from -128 to 255
- `toupper` (384 edge, 0 random of 0): toupper of an int from -128 to 255

## Left out

- `qsort`: game logic has no qsort call, and the order in which equal elements end is left open.
