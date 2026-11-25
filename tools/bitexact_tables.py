"""Generate the bit-exact library's tables as C source, from exact integer arithmetic alone.

    uv run tools/bitexact_tables.py [output.c]

The default output is build/bitexact/bx_tables.c. The tables are:

- The sine table: 640 signed 16-bit entries, entry i being
  round(sin(i * 2 * pi / 512) * 0x2000); the cosine is the same table read a
  quarter turn (128 entries) on.
- Two doubles, the decimal constants 10430.37835047 and 9.587379924285e-05
  (the conversion factors between a 16-bit angle, 0x10000 being a full turn,
  and radians; not 0x8000 / pi and pi / 0x8000, which differ in the low bits).
- The sine and cosine of every 16-bit angle (65536 entries, the angle taken
  as signed): the correctly rounded 64-bit mantissa of sin and cos of
  angle * 9.587379924285e-05, as an 80-bit extended value (mantissa high word,
  mantissa low word) and a sign/exponent word, computed in exact integer
  arithmetic. The same arithmetic gives atan(k / 16) for k = 0..16 as a
  double-double (a pair of doubles), the 12 coefficients (-1)^m / (2m + 1) of
  the arctangent series as double-doubles, and pi and pi / 2 as double-doubles.
- The powers of ten 10^n and 10^-n, for n = 1..7, 8..56 in steps of 8 and so
  on by powers of 8 up to 4096: two tables of 29 entries each, every entry
  12 bytes, an 80-bit mantissa rounded to nearest and an exponent word. An
  entry whose low 16 bits are 0x8000 or more is stored one unit of the 64-bit
  mantissa too high.
"""

import math
import struct
import sys
from fractions import Fraction
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

OUT = ROOT / "build/bitexact/bx_tables.c"
SIN_TABLE_LEN = 640
POW10_ENTRIES = 29
ATAN_SERIES_TERMS = 12

PREC = 256
ONE = 1 << PREC


def sincos_scaled(theta: float) -> tuple[int, int]:
    f = Fraction(theta)
    x = f.numerator * ONE // f.denominator
    x2 = x * x // ONE
    sin = term = x
    cos = tcos = ONE
    n = 1
    while term or tcos:
        tcos = -tcos * x2 // ONE // ((2 * n - 1) * (2 * n))
        term = -term * x2 // ONE // ((2 * n) * (2 * n + 1))
        cos += tcos
        sin += term
        n += 1
    return sin, cos


def to_x87(v: int) -> tuple[int, int, int]:
    """(mantissa high word, low word, sign/exponent word) of v / ONE as an 80-bit extended."""
    if v == 0:
        return 0, 0, 0
    sign = 0x8000 if v < 0 else 0
    a = abs(v)
    e = a.bit_length()
    shift = e - 64
    m, rem = divmod(a, 1 << shift)
    half = 1 << (shift - 1)
    if rem > half or (rem == half and m & 1):
        m += 1
    if m == 1 << 64:
        m >>= 1
        e += 1
    exp = e - PREC - 1 + 16383
    return m >> 32, m & 0xFFFFFFFF, sign | exp


def atan_scaled(x: int) -> int:
    for _ in range(6):
        x = x * ONE // (ONE + math.isqrt(ONE * ONE + x * x))
    x2 = x * x // ONE
    total = term = x
    n = 1
    while term:
        term = -term * x2 // ONE
        total += term // (2 * n + 1)
        n += 1
    return total * 64


def double_bits(d: float) -> tuple[int, int]:
    b = struct.unpack("<Q", struct.pack("<d", d))[0]
    return b >> 32, b & 0xFFFFFFFF


def double_double(v: int) -> tuple[tuple[int, int], tuple[int, int]]:
    f = Fraction(v, ONE)
    hi = float(f)
    lo = float(f - Fraction(hi))
    return double_bits(hi), double_bits(lo)


def pow10_entry(n: int, negative: bool) -> bytes:
    """The 12 bytes of the table entry for 10^n or 10^-n."""
    v = Fraction(10) ** (-n if negative else n)
    shift = 79 - (v.numerator.bit_length() - v.denominator.bit_length())
    for s in (shift - 1, shift, shift + 1, shift + 2):
        x = v * Fraction(2) ** s
        if (x.numerator // x.denominator).bit_length() == 80:
            shift = s
            break
    x = v * Fraction(2) ** shift
    m, rem = divmod(x.numerator, x.denominator)
    if 2 * rem > x.denominator or (2 * rem == x.denominator and m & 1):
        m += 1
    exp = 0x3FFF + 79 - shift
    if m.bit_length() == 81:
        m >>= 1
        exp += 1
    if m & 0xFFFF >= 0x8000:
        m += 0x10000
    return (m | exp << 80).to_bytes(12, "little")


def pow10_table(negative: bool) -> bytes:
    return b"".join(pow10_entry((i % 7 + 1) * 8 ** (i // 7), negative) for i in range(POW10_ENTRIES))


def sin_table() -> list[int]:
    return [round(math.sin(i * 2 * math.pi / 512) * 0x2000) for i in range(SIN_TABLE_LEN)]


def angle_doubles() -> tuple[float, float]:
    return 10430.37835047, 9.587379924285e-05


def rows(values: list[str], per_line: int) -> str:
    return "".join(
        "    " + ", ".join(values[i:i + per_line]) + (",\n" if i + per_line < len(values) else "\n")
        for i in range(0, len(values), per_line))


def generate(out_path: Path) -> None:
    sin = sin_table()
    rad_to_angle16, angle16_to_rad = angle_doubles()

    out = ['#include "bx_tables.h"\n\n']
    out.append(f"const bx_i16 bx_sin_table[{len(sin)}] = {{\n{rows([str(v) for v in sin], 16)}}};\n\n")
    for name, d in (("bx_rad_to_angle16_bits", rad_to_angle16), ("bx_angle16_to_rad_bits", angle16_to_rad)):
        hi, lo = double_bits(d)
        out.append(f"const bx_u32 {name}[2] = {{0x{hi:08x}u, 0x{lo:08x}u}};\n")
    out.append("\n")

    atans = []
    for k in range(17):
        v = atan_scaled(k * ONE // 16)
        if k == 16:
            pi_half = Fraction(2 * v, ONE)
            assert abs(float(pi_half) * 2 - math.pi) < 1e-15
            pi_dd = double_double(4 * v)
            half_dd = double_double(2 * v)
        assert abs(float(Fraction(v, ONE)) - math.atan(k / 16)) < 1e-15
        atans.append(double_double(v))
    vals = []
    for hi, lo in atans:
        vals += [f"0x{hi[0]:08x}u", f"0x{hi[1]:08x}u", f"0x{lo[0]:08x}u", f"0x{lo[1]:08x}u"]
    out.append("const bx_u32 bx_atan_sixteenths[17 * 4] = {\n" + rows(vals, 4) + "};\n\n")
    vals = []
    for m in range(ATAN_SERIES_TERMS):
        f = Fraction((-1) ** m, 2 * m + 1)
        hi = float(f)
        lo = float(f - Fraction(hi))
        assert abs(Fraction(hi) + Fraction(lo) - f) < Fraction(1, 1 << 100)
        vals += [f"0x{w:08x}u" for w in (*double_bits(hi), *double_bits(lo))]
    out.append(f"const bx_u32 bx_atan_series[{ATAN_SERIES_TERMS} * 4] = {{\n" + rows(vals, 4) + "};\n\n")
    vals = []
    for hi, lo in (pi_dd, half_dd):
        vals += [f"0x{hi[0]:08x}u", f"0x{hi[1]:08x}u", f"0x{lo[0]:08x}u", f"0x{lo[1]:08x}u"]
    out.append("const bx_u32 bx_pi_pi_half[2 * 4] = {\n" + rows(vals, 4) + "};\n\n")

    for name, negative in (("bx_pow10_pos", False), ("bx_pow10_neg", True)):
        raw = pow10_table(negative)
        words = struct.unpack(f"<{len(raw) // 4}I", raw)
        out.append(f"const bx_u32 {name}[{POW10_ENTRIES} * 3] = {{\n{rows([f'0x{w:08x}u' for w in words], 3)}}};\n\n")

    vals = []
    for idx in range(65536):
        a = idx - 65536 if idx >= 32768 else idx
        s, c = sincos_scaled(a * angle16_to_rad)
        sh, sl, se = to_x87(s)
        ch, cl, ce = to_x87(c)
        vals += [f"0x{sh:08x}u", f"0x{sl:08x}u", f"0x{ch:08x}u", f"0x{cl:08x}u", f"0x{se:04x}{ce:04x}u"]
    out.append("const bx_u32 bx_sincos_table[65536 * 5] = {\n" + rows(vals, 5) + "};\n")
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text("".join(out))
    print(f"wrote {out_path.relative_to(ROOT) if out_path.is_relative_to(ROOT) else out_path} ({out_path.stat().st_size:,} bytes)")


def main() -> None:
    generate(Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else OUT)


if __name__ == "__main__":
    main()
