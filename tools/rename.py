"""Rename identifiers everywhere they are spelt.

    uv run tools/rename.py OLD NEW [OLD NEW ...]   # rename
    uv run tools/rename.py --from FILE             # the pairs from a CSV (old,new[,evidence])
    uv run tools/rename.py OLD NEW --dry-run       # what would change, and what is refused
    uv run tools/rename.py --from FILE --join --keep-going
                                                   # several views to one type, skipping the
                                                   # pairs that are refused
    uv run tools/test_rename.py                    # tests, on a fixture tree

Placeholder names (`Class_<address>`, `FUN_<address>`, `DAT_<address>`, the
`X_<address>` views, `field_<offset>`) become real ones where the evidence gives
them, and the several placeholder names one type has across the files become that
type's one name. A function's or a class's name is part of the decorated names the
linker matches, so every file that spells it changes at once.

What it rewrites:

  src, include, interfaces,   every .c, .cpp and .h: OLD as a whole word in code
  lib, cmake, link, tests     and in comments, never inside a string or character
                              literal; in the decorated symbol of a `// FUNCTION:`
                              or `// ENTRY:` annotation too. The generated
                              src/harness/trace_schema.h and
                              src/state/layout_asserts.cpp are among them.
                              `bx_OLD`, the name lib/bitexact gives a function
                              (bx_FUN_004b70ef), becomes `bx_NEW` with it.
  data/aliases.csv            inside the names, decorated or not
  data/symbols.csv
  data/*.csv (CELLS)          the cells that hold a function, global, field or
                              decorated name, in place: the hand-set columns of
                              state_classes.csv, trace_coverage.csv, hazards.csv
                              and the like are keyed by them. Tables a tool
                              regenerates entirely (rwsets.csv, coupling.csv,
                              type_dupes.csv, layout_refs.csv, outside_pointers.csv)
                              are left to it: the commands to run follow a rename.
  docs/*.md, AGENTS.md,       OLD as a whole word, but not in a --join: there
  lib/bitexact/README.md      the docs keep the view names they tell apart

A gap entry label (`// ENTRY: 0x4b70ef`) has no name of its own: tools/aliases.py makes
the public symbol `_FUN_004b70ef` from its address at link time. Renaming FUN_004b70ef
writes the new name into the annotation (`// ENTRY: 0x4b70ef _NEW`), so the link makes
`_NEW` and the callers still find it.

What it refuses, pair by pair, because the modern build or conformity would break:

  - a NEW that is not an identifier, or is a C++ keyword;
  - two names of one file that would become one: a file spelling OLD that
    already spells NEW, or another OLD renamed to the same NEW. Two member
    functions of different classes may share a name (data/symbols.csv says
    which class a method belongs to: `Class::FUN_<address>`); two of one
    class would be overloads, and a free function or a type in the way is
    still refused (merging two types in one file is not a rename);
  - a NEW that data/symbols.csv gives another address after the rename;
  - a NEW that is already a type in other files, unless --join says OLD's
    views are views of that type;

Any refusal stops the rename, unless --keep-going, which renames the pairs
that pass and lists the others. A real run ends with the commands that
regenerate the tables and generated files the rename touched.
"""

import argparse
import csv
import io
import re
import sys
from collections import defaultdict
from pathlib import Path

from sources import ROOT

csv.field_size_limit(1 << 28)

WORD = r"[A-Za-z0-9_]"
IDENT = re.compile(r"[A-Za-z_][A-Za-z0-9_]*$")
TOKEN = re.compile(r"[A-Za-z_][A-Za-z0-9_]*")
KEYWORDS = set("""
    asm auto bool break case catch char class const const_cast continue default delete do double
    dynamic_cast else enum explicit export extern false float for friend goto if inline int long
    mutable namespace new operator private protected public register reinterpret_cast return short
    signed sizeof static static_cast struct switch template this throw true try typedef typeid
    typename union unsigned using virtual void volatile wchar_t while
    __asm __cdecl __stdcall __fastcall __thiscall __declspec __try __except __finally __leave __int64
""".split())
CODE_DIRS = ["src", "include", "interfaces", "lib", "cmake", "link", "tests"]
CODE_SUFFIXES = {".c", ".cpp", ".h"}
TEXT_FILES = ["AGENTS.md", "docs/*.md", "lib/bitexact/README.md"]
NAME_TABLES = ["data/aliases.csv", "data/symbols.csv"]
CELLS = {
    "data/state_classes.csv": [("name", "words")],
    "data/trace_coverage.csv": [("name", "words")],
    "data/globals.csv": [("name", "words")],
    "data/functions.csv": [("name", "words")],
    "data/hazards.csv": [("function", "words"), ("target", "words")],
    "data/hazard_edges.csv": [("caller", "words"), ("target", "words")],
    "data/subsystems.csv": [("function", "words")],
    "data/game_logic.csv": [("function", "words")],
    "data/crt_calls.csv": [("function", "words")],
    "data/x87_sites.csv": [("function", "words")],
    "data/quirk_candidates.csv": [("function", "words")],
    "data/simalloc_allow.csv": [("name", "words")],
    "data/ref_exceptions.csv": [("struct", "words"), ("field", "words")],
    "data/initialiser_allow.csv": [("refs", "symbols")],
    "data/progress.csv": [("symbol", "symbols")],
    "data/block_pointers.csv": [("holder", "symbols"), ("target", "symbols")],
}
ANNOTATED = re.compile(r"(//\s*FUNCTION:\s*0x[0-9a-fA-F]+[ \t]+)(\S+)")
ENTRY_LABEL = re.compile(r"(//\s*ENTRY:\s*0x([0-9a-fA-F]+))(?:([ \t]+)(\S+))?([ \t]*\r?)$")
TRAILING = re.compile(r"^(.*?\S)(\s{2,})(//.*?)(\r?)$")


# --- the scanner ------------------------------------------------------------------------

def segments(text: str) -> list[tuple[str, str]]:
    """The text cut into ("code" | "comment" | "literal", text) pieces, in order."""
    out, i, n, start, kind = [], 0, len(text), 0, "code"

    def cut(at: int, new_kind: str) -> None:
        nonlocal start, kind
        if at > start:
            out.append((kind, text[start:at]))
        start, kind = at, new_kind

    while i < n:
        c = text[i]
        if text.startswith("//", i):
            cut(i, "comment")
            end = text.find("\n", i)
            i = n if end < 0 else end
            cut(i, "code")
            continue
        if text.startswith("/*", i):
            cut(i, "comment")
            end = text.find("*/", i + 2)
            i = n if end < 0 else end + 2
            cut(i, "code")
            continue
        if c in "\"'":
            cut(i, "literal")
            j = i + 1
            while j < n and text[j] != c and text[j] != "\n":
                j += 2 if text[j] == "\\" else 1
            i = min(j + 1, n)
            cut(i, "code")
            continue
        i += 1
    cut(n, "code")
    return out


def code_of(text: str) -> str:
    return "".join(piece if kind == "code" else " " * len(piece) for kind, piece in segments(text))


class Renamer:
    """Every pair at once: one regex for whole words, one for names inside
    decorated symbols (after a non-word character, after the code of a
    class V, struct U, union T or enum W4: `PAUUnit@@`, `IURect_0046e160::`,
    or after the code of a special member: `??0` and `??1` the constructor and
    destructor, `??_G` and `??_E` the deleting destructors, `??_7` the vtable,
    `??_R2` to `??_R4` the RTTI records)."""

    def __init__(self, pairs: list[tuple[str, str]]):
        self.table = dict(pairs)
        names = "|".join(re.escape(o) for o in sorted(self.table, key=len, reverse=True))
        self.word = re.compile(rf"(?<!{WORD})(?:{names})(?!{WORD})")
        self.decorated = re.compile(rf"(?:(?<!{WORD})|(?<=[UVT4])|(?<=\?\?[01])|(?<=\?\?_[GE789DF])"
                                    rf"|(?<=\?\?_R[234]))(?:{names})(?!{WORD})")

    def words(self, text: str) -> str:
        return self.word.sub(lambda m: self.table[m.group(0)], text)

    def symbols(self, text: str) -> str:
        return self.decorated.sub(lambda m: self.table[m.group(0)], text)

    def entry(self, m: re.Match) -> str:
        """An `// ENTRY:` label with its symbol renamed, or, when it is unnamed and OLD is the
        FUN_<address> that tools/aliases.py would make its symbol from, named NEW."""
        head, address, gap, symbol, tail = m.groups()
        if symbol is None:
            new = self.table.get(f"FUN_{int(address, 16):08x}")
            return m.group(0) if new is None else f"{head} _{new}{tail}"
        return head + gap + "_" * symbol.startswith("_") + self.symbols(symbol.lstrip("_")) + tail

    def source(self, text: str) -> str:
        out = []
        for kind, piece in segments(text):
            if kind == "comment":
                piece = ANNOTATED.sub(lambda m: m.group(1) + self.symbols(m.group(2)), piece)
                piece = ENTRY_LABEL.sub(self.entry, piece)
            if kind != "literal":
                piece = self.words(piece)
            out.append(piece)
        new = "".join(out)
        if new == text:
            return text
        # Keep trailing comments in their column where a name got shorter or longer.
        old_lines, new_lines = text.split("\n"), new.split("\n")
        for i, (o, n) in enumerate(zip(old_lines, new_lines)):
            if o == n:
                continue
            mo, mn = TRAILING.match(o), TRAILING.match(n)
            if mo and mn and mo.group(3) == mn.group(3):
                column = len(mo.group(1)) + len(mo.group(2))
                new_lines[i] = mn.group(1) + " " * max(2, column - len(mn.group(1))) + mn.group(3) + mn.group(4)
        return "\n".join(new_lines)


# --- what is refused ------------------------------------------------------------------------

class Tree:
    """What the refusals need from every source file, read once."""

    def __init__(self, texts: dict[Path, str]):
        self.texts = texts
        self.headers = {p for p in texts if p.suffix == ".h"}
        self.codes = {p: code_of(t) for p, t in texts.items()}
        self.tokens = {p: set(TOKEN.findall(c)) for p, c in self.codes.items()}
        # A file that includes one of the generated headers sees its names too,
        # unless it includes it inside a namespace (`namespace ta { #include ... }`,
        # for the header's symbol count alone), where they cannot clash with its own.
        self.included: dict[Path, set[str]] = defaultdict(set)
        for p, text in texts.items():
            for m in re.finditer(r'^\s*#\s*include\s*["<]([^">]+)[">]', text, re.M):
                if re.search(r"\bnamespace\s+\w+\s*\{\s*$", text[:m.start()]):
                    continue
                header = ROOT / "include" / m.group(1)
                if header in self.tokens and header != p:
                    self.included[p] |= self.tokens[header]
        self.types: dict[str, list[Path]] = defaultdict(list)
        for p, c in self.codes.items():
            for m in re.finditer(r"\b(?:struct|class|union|enum)\s+([A-Za-z_]\w*)", c):
                if p not in self.types[m.group(1)]:
                    self.types[m.group(1)].append(p)


def symbols_rows() -> list[dict]:
    path = ROOT / "data/symbols.csv"
    return list(csv.DictReader(path.open())) if path.exists() else []


def refusals(pairs: list[tuple[str, str]], tree: Tree, join: bool) -> dict[tuple[str, str], list[str]]:
    """The reasons each refused pair is refused."""
    out: dict[tuple[str, str], list[str]] = defaultdict(list)
    olds = {o for o, _ in pairs}
    groups: dict[str, set[str]] = defaultdict(set)
    for old, new in pairs:
        groups[new].add(old)
    rel = lambda p: str(p.relative_to(ROOT))  # noqa: E731
    # Which class each name is a method of, from data/symbols.csv's `Class::FUN_`
    # rows: the one clash a rename allows below is between methods of different
    # classes. A name the table gives no class is no method (a free function, a
    # type, a global).
    rows = symbols_rows()
    members: dict[str, set[str]] = defaultdict(set)
    plain: set[str] = set()
    for r in rows:
        cls, sep, name = r["name"].rpartition("::")
        if sep and IDENT.match(name):
            members[name].add(cls)
        elif IDENT.match(r["name"]):
            plain.add(r["name"])

    def allowed_clash(first: str, second: str) -> bool:
        """Whether two names may become one: both are methods, of different
        classes. Two of one class would be overloads, and a free function or a
        type in the way keeps the clash refused (merging types is phase 3)."""
        if first in tree.types or second in tree.types:
            return False
        a = None if first in plain else members.get(first)
        b = None if second in plain else members.get(second)
        return bool(a) and bool(b) and not (a & b)

    for old, new in pairs:
        if not IDENT.match(new) or new in KEYWORDS:
            out[(old, new)].append(f"{new!r} is not a name C++ allows")
        if not IDENT.match(old):
            out[(old, new)].append(f"{old!r} is not an identifier")
        others = (groups[new] - {old}) | ({new} if new not in olds else set())
        others = {o for o in others if not allowed_clash(old, o)}
        clash = [p for p, toks in tree.tokens.items()
                 if old in toks and (toks | tree.included[p]) & others and p not in tree.headers]
        if clash:
            seen_there = others & (tree.tokens[clash[0]] | tree.included[clash[0]])
            out[(old, new)].append(f"{old} and {' / '.join(sorted(seen_there))} "
                                   f"would both be {new} in {rel(clash[0])}"
                                   + (f" and {len(clash) - 1} more" if len(clash) > 1 else ""))
        types = [p for p in tree.types.get(new, []) if old not in tree.tokens[p]]
        if types and not join and new not in olds:
            out[(old, new)].append(f"{new} is already a type in {len(types)} file(s) ({rel(types[0])}, ...): "
                                   f"renaming {old} to it makes their views one type; pass --join if the "
                                   f"evidence says they are")
    # The name table after the rename: one address per name.
    if rows:
        renamer = Renamer(pairs)
        seen: dict[str, tuple[str, str]] = {}
        for r in rows:
            name = renamer.symbols(r["name"])
            if name in seen and seen[name][0] != r["address"] and (name != r["name"] or seen[name][1] != name):
                culprits = [pr for pr in pairs if pr[1] in name]
                for pr in culprits or pairs[:1]:
                    out[pr].append(f"data/symbols.csv would give {name} two addresses "
                                   f"({seen[name][0]}, {r['address']})")
            seen.setdefault(name, (r["address"], r["name"]))
    return out


# --- the rewrite and the checks ------------------------------------------------------------

def sources_and_headers() -> list[Path]:
    return sorted(p for d in CODE_DIRS for p in (ROOT / d).rglob("*") if p.suffix in CODE_SUFFIXES and p.is_file())


def rename_cells(text: str, columns: list[tuple[str, str]], renamer: "Renamer") -> str:
    """A CSV with the renamer applied to the cells of some columns, the rest byte for byte."""
    eol = "\r\n" if "\r\n" in text else "\n"
    rows = list(csv.reader(io.StringIO(text, newline="")))
    if not rows:
        return text
    header = rows[0]
    todo = [(header.index(c), renamer.words if mode == "words" else renamer.symbols)
            for c, mode in columns if c in header]
    changed = False
    for row in rows[1:]:
        for i, convert in todo:
            if i < len(row) and (new := convert(row[i])) != row[i]:
                row[i], changed = new, True
    if not changed:
        return text
    out = io.StringIO(newline="")
    csv.writer(out, lineterminator=eol).writerows(rows)
    return out.getvalue()


def apply(pairs: list[tuple[str, str]], texts: dict[Path, str], dry_run: bool, docs: bool = True) -> list[str]:
    renamer = Renamer(pairs)
    changed: list[str] = []

    def save(path: Path, before: str, after: str) -> None:
        if after != before:
            changed.append(str(path.relative_to(ROOT)))
            if not dry_run:
                path.write_bytes(after.encode("latin-1"))

    for path, text in texts.items():
        save(path, text, renamer.source(text))
    for pattern in NAME_TABLES:
        path = ROOT / pattern
        if path.exists():
            text = path.read_bytes().decode("latin-1")
            save(path, text, renamer.symbols(text))
    for pattern, columns in CELLS.items():
        path = ROOT / pattern
        if path.exists():
            text = path.read_bytes().decode("latin-1")
            save(path, text, rename_cells(text, columns, renamer))
    for pattern in TEXT_FILES if docs else []:
        for path in sorted(ROOT.glob(pattern)):
            text = path.read_bytes().decode("latin-1")
            save(path, text, renamer.words(text))
    return changed


FOLLOW_UP = [
    "cmake --build build/modern --target game data bitexact",
    "uv run tools/traceschema.py",
    "uv run tools/layoutasserts.py --write --build-dir build/modern",
    "uv run tools/rwsets.py",
    "uv run tools/hazards.py",
    "uv run tools/coupling.py",
    "uv run tools/stateclasses.py",
    "uv run tools/refcheck.py --write",
    "uv run tools/typedupes.py",
]


def implied(pairs: list[tuple[str, str]], tree: "Tree") -> dict[tuple[str, str], tuple[str, str]]:
    """The `bx_OLD` -> `bx_NEW` pairs for the renamed names lib/bitexact spells that way, by their pair."""
    seen = set().union(*tree.tokens.values()) if tree.tokens else set()
    return {(f"bx_{o}", f"bx_{n}"): (o, n) for o, n in pairs if f"bx_{o}" in seen}


def main() -> None:
    global ROOT
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("names", nargs="*", help="OLD NEW pairs")
    ap.add_argument("--from", dest="table", type=Path, help="a CSV of old,new[,evidence] rows")
    ap.add_argument("--dry-run", action="store_true", help="print what would change and stop")
    ap.add_argument("--join", action="store_true",
                    help="allow a NEW that is already a type elsewhere: OLD's views join that type")
    ap.add_argument("--keep-going", action="store_true", help="rename the pairs that pass, list the others")
    ap.add_argument("--root", type=Path, default=ROOT, help="the tree to rename in (default: this one)")
    args = ap.parse_args()
    ROOT = args.root.resolve()
    if len(args.names) % 2:
        ap.error("names come in OLD NEW pairs")
    pairs = list(zip(args.names[::2], args.names[1::2]))
    if args.table:
        with args.table.open() as fh:
            pairs += [(r["old"], r["new"]) for r in csv.DictReader(fh)]
    if not pairs:
        ap.error("nothing to rename")
    if len({o for o, _ in pairs}) != len(pairs):
        sys.exit("an OLD name appears twice among the pairs")
    texts = {p: p.read_bytes().decode("latin-1") for p in sources_and_headers()}
    tree = Tree(texts)
    while True:
        extra = implied(pairs, tree)
        base_of = {pr: pr for pr in pairs} | extra
        refused = refusals(pairs + list(extra), tree, args.join)
        if not refused:
            break
        print(f"refused {len(refused)} of {len(base_of)} pair(s):")
        for (old, new), why in sorted(refused.items()):
            print(f"  {old} -> {new}: {'; '.join(dict.fromkeys(why))}")
        if not args.keep_going:
            sys.exit(2)
        # Without the refused pairs, others may now pass (or clash): look again.
        gone = {base_of[pr] for pr in refused}
        pairs = [p for p in pairs if p not in gone]
        if not pairs:
            sys.exit(2)
    pairs += list(extra)
    # A join gives many views one name: the docs keep the names they tell apart.
    changed = apply(pairs, texts, args.dry_run, docs=not args.join)
    for f in changed:
        print(f)
    print(f"{len(pairs)} pair(s): {'would change' if args.dry_run else 'changed'} {len(changed)} file(s)")
    if changed and not args.dry_run:
        print("Regenerate, in this order, then run the --check of each:")
        for cmd in FOLLOW_UP:
            print(f"  {cmd}")


if __name__ == "__main__":
    main()
