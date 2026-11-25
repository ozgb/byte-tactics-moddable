"""Write and check the kit's runtime vectors, tests/runtime/vectors/ (#1060).

    uv run tools/runtimevectors.py write     # needs Wine, clang-cl and the vectors of tools/bitexact.py
    uv run tools/runtimevectors.py check     # the committed directory, standard library only
    uv run tools/test_runtimevectors.py      # tests, on fixture directories

`write` runs `tools/bitexact.py --kit DIR`, which replays the original's recorded results and writes every case
(its inputs and the recorded outputs) of each runtime entry to DIR as `<entry>.edge.bin`, `<entry>.random.bin` and
`<entry>.layout.txt`. It then writes tests/runtime/vectors/: one `<entry>.bin` per entry (the edge records, then a
sample of the random records), `vectors.json` (the manifest), `layouts.txt` (one line per entry: its name, its input
types and its output types, comma separated, for readers without a JSON parser) and `README.md` (the record format,
the sampling rule and one line per entry). The directory is exported to the data kit as `runtime/vectors/`.

The directory stays within BUDGET bytes. The edge records of the library's own entries (LIBRARY) are kept whole. The
random records of every entry are sampled to RANDOM_CAP bytes by a seeded rule (`strata`), and the edge records of
the C semantics entries to EDGE_CAP bytes: the first EDGE_PREFIX bytes of records, which are the named special cases,
then a sample of the rest by the same rule.

`check` fails (exit 1, one line per problem) when the manifest does not parse, a file's size is not its records'
sizes, a count differs from the manifest, the budget is exceeded, a file is not in the manifest, or the README does
not name every entry.
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
EDGE_CAP = 120_000
EDGE_PREFIX = 60_000
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
    "sine": "the sine of a 16-bit angle (65536 is a full turn) as a fixed-point int, 8192 being 1.0; every angle is an edge record",
    "cosine": "the cosine of a 16-bit angle in the same fixed point; every angle is an edge record",
    "mul_hi": "the high 32 bits of the signed 64-bit product of two ints",
    "scaled_sine": "scale times the sine of an angle, the angle rounded to a table step and the product rounded to nearest, as an int",
    "scaled_cosine": "scale times the cosine of an angle, in the same rounding",
    "angle_of_vector": "the angle (32768 is half a turn) of the vector (z, x), the arctangent of x over z, rounded to nearest even",
    "rotate_point": "the point (x, z) rotated by an angle (rx, rz); an angle of 0 leaves it, a result outside the int range is -2147483648",
    "rotation_matrix": "the 3x3 rotation matrix (m0 to m8, row by row) built from three angles",
    "apply_rotation": "a vector (x, y, z) times a 3x3 matrix (m0 to m8, row by row), giving (rx, ry, rz); `aliased` is 1 when the result overwrites the vector it is computed from",
    "mul_div": "a times b divided by c, the product taken in 64 bits and the quotient truncated toward zero, 0 when c is 0; a case whose quotient does not fit an int is not recorded",
    "sqrt": "the square root of a non-negative double",
    "sqrt_float": "the square root of a non-negative float, given and returned as its 32-bit pattern",
    "hypot": "the length of the vector (x, y) by the C runtime's hypot, finite arguments: big is the larger magnitude, the result is the square root of (x / big)^2 + (y / big)^2, times big, each step rounded to a 64-bit mantissa and stored as a double",
    "acos": "the arccosine of x in [-1, 1] as a double; a case the processor's arctangent rounds differently from the library's own result is dropped",
    "float_text": "the float the C runtime's scanf conversion `%f` stores for a text, as the float's 32-bit pattern; it is not always the correctly rounded float",
    "double_text": "the double the C runtime's atof returns for a text: leading white space skipped, any trailing text ignored, overflow to infinity, underflow to zero or a denormal",
    "rand": "the first 10000 values of the C runtime's rand after srand(seed), as 10000 little endian i32 (40000 bytes)",
    "shuffle": "the ints 1, 4, 7, ... (3 * index + 1) for size elements after the C++ library's random_shuffle with rand seeded by `seed`, as `size` little endian i32 behind a u32 byte count, then the next four values of rand",
    "load_sequence": "the load sequence: srand(seed), the 391606 rand draws the explosion setup makes, then a random_shuffle of the first `size` (3 to 10) of the ints 0 to 9, as 10 little endian i32, then the next four values of rand",
    "atoi": "atoi of a string: white space of every kind, signs, leading zeros, trailing text, values around and past the int range and 2^32, high bytes",
    "itoa": "_itoa of a value in a radix: the text it writes",
    "strcmpi": "_strcmpi of two strings: the sign (-1, 0, 1) and the raw result",
    "strnicmp": "_strnicmp of two strings and a count: the sign and the raw result",
    "strncmp": "strncmp of two strings and a count: the sign and the raw result",
    "tolower": "tolower of an int from -128 to 255",
    "toupper": "toupper of an int from -128 to 255",
    "isspace": "isspace of an int from -1 to 255, the raw result; the results for -128 to -2 depend on memory before the library's table and are dropped",
    "isdigit": "isdigit of an int from -1 to 255, the raw result; the results for -128 to -2 are dropped",
    "isalnum": "isalnum of an int from -1 to 255, the raw result; the results for -128 to -2 are dropped",
    "strlwr": "_strlwr of a string: the string after it",
    "strncpy": "strncpy of a source string and a count into a 24 byte destination filled with 0xAA beforehand: all 24 bytes after it, so the zero fill shows",
    "strstr": "strstr of a string and a needle: the offset of the match, or -1",
    "strchr": "strchr of a string and a character (0 included): the offset of the match, or -1",
    "strrchr": "strrchr of a string and a character (0 included): the offset of the match, or -1",
    "strcspn": "strcspn of a string and a set of characters, either possibly empty: the length of the prefix without them",
    "strtok": "strtok over a string in a 48 byte buffer filled with 0xAA beforehand, the first call with `delims`, the next calls with `delims2` until it returns null: the token count, `toks` as 24 pairs of i32 (offset in the buffer, length) of which the count's worth are used, then -1 (a null) or -2 and the offset a further call returns, and the buffer afterward",
    "memmove": "memmove within a 64 byte buffer holding byte i = (37 * i + 11) mod 256: the offset of the returned pointer and the buffer afterward, for overlapping and disjoint ranges",
    "sprintf": "sprintf of a format with arguments: `kinds` has one letter per conversion argument in order (i an int taken from `ints`, f a double from `dbls`, s a string from `strs`, each indexed by the argument's position; `ints` is 8 little endian i32, `dbls` 8 f64, `strs` 8 strings of 64 bytes), and the outputs are the text written and the return value",
    "sscanf": "sscanf of a text with a format whose conversions are integers, words and sets (none is `%f`): the return value, the four ints stored (0x5a5a5a5a where nothing was stored) and the 64 bytes of the string buffer (0xAA where nothing was stored)",
    "ftol": "the conversion of a double to an int by the compiled code of the original program (a call to its _ftol); the result is the one the original gave, including for values out of range and for NaN",
}

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
cases left out because the result is not one a runtime can be held to (it is 7 for `acos`, where the processor's
arctangent is not always the correctly rounded value, and 127 for each of the three character classes, whose results below -1
depend on memory).

## Sampling

The library entries (sine to load_sequence below) keep every edge record. The other entries keep their first records
whole up to 60000 bytes, which are the named special cases, and then a sample of the rest up to 120000 bytes in all.
The random records of every entry are sampled to 80000 bytes, or kept whole when they are fewer. A sample of m of n
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


def edge_records(data, types: list[str], whole: bool) -> tuple[list[bytes], int]:
    """The edge records to keep and the number of edge records in all: every one when whole or when they fit
    EDGE_CAP, else the first EDGE_PREFIX bytes of records and a sample of the rest."""
    if len(data) == 0:
        return [], 0
    records = pick(data, types, 0, list(range(count_records(data, types))))
    if whole or len(data) <= EDGE_CAP:
        return records, len(records)
    prefix = []
    used = 0
    for r in records:
        if used + len(r) > EDGE_PREFIX:
            break
        prefix.append(r)
        used += len(r)
    rest = records[len(prefix):]
    rest_bytes = sum(len(r) for r in rest)
    if used + rest_bytes <= EDGE_CAP:
        return records, len(records)
    m = max(1, min(len(rest), (EDGE_CAP - used) * len(rest) // rest_bytes))
    return prefix + [rest[i] for i in strata(len(rest), m)], len(records)


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
        edge, edge_total = edge_records(edge_data, types, name in LIBRARY)
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


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    sub = parser.add_subparsers(dest="command", required=True)
    write = sub.add_parser("write", help="run the kit and write tests/runtime/vectors/")
    write.add_argument("--scratch", type=Path, default=SCRATCH, help=f"where the kit run writes (default {SCRATCH})")
    write.add_argument("--reuse", action="store_true", help="use the kit files already in --scratch")
    write.add_argument("--out", type=Path, default=OUT)
    verify = sub.add_parser("check", help="check the committed directory")
    verify.add_argument("--dir", type=Path, default=OUT)
    args = parser.parse_args(argv)
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
    problems = check(args.dir)
    for p in problems:
        print(p, file=sys.stderr)
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
