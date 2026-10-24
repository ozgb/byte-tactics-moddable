"""Rename identifiers everywhere they are spelt.

    uv run tools/rename.py OLD NEW [OLD NEW ...]   # rename
    uv run tools/rename.py --from FILE             # the pairs from a CSV (old,new[,evidence])
    uv run tools/rename.py Type::member NEW        # one type's member only, see below
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

A member shared by several types (`field_0` is in a dozen) is renamed for one type
with the row `Type::member,newName[,evidence]` (or `Type::member NEW` on the command
line). Only what is proven to belong to `Type` changes:

  - the member's declaration in the definition of `Type` (struct, class or union,
    through the anonymous unions and structs inside it, not a nested named type),
    and the uses of the bare name in its member functions, in the body or out of
    class as `Type::Name(...)`, where `this->member` counts too;
  - `offsetof(Type, member)` and the `"member"` message of the static_assert
    around it;
  - `root->member` and `root.member` where `root` is a name declared `Type`, `Type*`
    or `Type&` in the same top-level function or declaration (parameters included),
    or a global declared that way in a file that spells `Type`, and is never
    declared as another type in that function. `((Type*)expr)->member` and
    `static_cast<Type*>(expr)->member` count as well;
  - in data/ref_exceptions.csv the `field` of the rows whose `struct` is `Type`;
    in state_classes.csv and trace_coverage.csv the `name` of the rows whose
    kind is the type's region (`game-region` for Game); and the spellings
    `Type::member` and `Type.member` in the other hand-set tables and the docs.

It refuses a `Type` with no definition, a `member` it does not declare, and a NEW
that is already a member of `Type`. The strings of the generated trace_schema.h are
for tools/traceschema.py to regenerate. Every other `x->member` or `x.member` that
is left is listed as a warning, by the expression before it, for a human to check:
the tool cannot tell whether it is a `Type`.

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
        old_lines, new_lines = text.split("\n"), new.split("\n")
        for i, (o, n) in enumerate(zip(old_lines, new_lines)):
            if o == n:
                continue
            mo, mn = TRAILING.match(o), TRAILING.match(n)
            if mo and mn and mo.group(3) == mn.group(3):
                column = len(mo.group(1)) + len(mo.group(2))
                new_lines[i] = mn.group(1) + " " * max(2, column - len(mn.group(1))) + mn.group(3) + mn.group(4)
        return "\n".join(new_lines)


class Tree:
    """What the refusals need from every source file, read once."""

    def __init__(self, texts: dict[Path, str]):
        self.texts = texts
        self.headers = {p for p in texts if p.suffix == ".h"}
        self.codes = {p: code_of(t) for p, t in texts.items()}
        self.tokens = {p: set(TOKEN.findall(c)) for p, c in self.codes.items()}
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

    def rel(p):
        return str(p.relative_to(ROOT))

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


def sources_and_headers() -> list[Path]:
    return sorted(p for d in CODE_DIRS for p in (ROOT / d).rglob("*") if p.suffix in CODE_SUFFIXES and p.is_file())


def edit_cells(text: str, edit) -> str:
    """A CSV with edit(header, row) applied to every data row (it returns whether it changed the row),
    the rest byte for byte."""
    eol = "\r\n" if "\r\n" in text else "\n"
    rows = list(csv.reader(io.StringIO(text, newline="")))
    if not rows:
        return text
    changed = False
    for row in rows[1:]:
        changed |= edit(rows[0], row)
    if not changed:
        return text
    out = io.StringIO(newline="")
    csv.writer(out, lineterminator=eol).writerows(rows)
    return out.getvalue()


def rename_cells(text: str, columns: list[tuple[str, str]], renamer: "Renamer") -> str:
    """A CSV with the renamer applied to the cells of some columns, the rest byte for byte."""
    def edit(header: list[str], row: list[str]) -> bool:
        changed = False
        for c, mode in columns:
            convert = renamer.words if mode == "words" else renamer.symbols
            if c in header and (i := header.index(c)) < len(row) and (new := convert(row[i])) != row[i]:
                row[i], changed = new, True
        return changed
    return edit_cells(text, edit)


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


SCOPED_NAME = re.compile(r"([A-Za-z_]\w*)::([A-Za-z_]\w*)$")
KIND_OF = {"Game": "game-region"}
ROW_KEYED = {"data/ref_exceptions.csv": ("struct", "field")}
KIND_KEYED = ["data/state_classes.csv", "data/trace_coverage.csv"]
GENERATED = {"rwsets", "coupling", "type_dupes", "layout_refs", "outside_pointers"}
QUALIFIERS = re.compile(r"\b(?:struct|class|union|enum|const|volatile|register|static|extern)\s+")
DECL = re.compile(r"\b([A-Za-z_]\w*)(?:\s*[*&]+\s*|\s+)([A-Za-z_]\w*)\s*(?=[,;=)\[])")
NOT_TYPES = {"return", "else", "delete", "goto", "case", "new", "throw", "sizeof", "typedef", "do", "operator"}
SEPARATORS = re.compile(r"[{};]")
WARN_LIMIT = 40


def matching(code: str, open_at: int) -> int:
    depth = 0
    for m in re.finditer(r"[{}]", code[open_at:]):
        depth += 1 if m.group(0) == "{" else -1
        if depth == 0:
            return open_at + m.start()
    return len(code)


def top_level(code: str) -> list[tuple[int, int, int]]:
    """The (start, end, open brace or -1) of every top-level block or statement, a block from the
    end of the one before it, so that its header and parameters are in it."""
    out, depth, boundary, opened = [], 0, 0, -1
    for m in SEPARATORS.finditer(code):
        c = m.group(0)
        if c == "{":
            if depth == 0:
                opened = m.start()
            depth += 1
        elif c == "}":
            depth = max(depth - 1, 0)
            if depth == 0:
                out.append((boundary, m.end(), opened))
                boundary = m.end()
        elif depth == 0:
            out.append((boundary, m.end(), -1))
            boundary = m.end()
    if boundary < len(code):
        out.append((boundary, len(code), -1))
    return out


def own_spans(code: str, lo: int, hi: int) -> list[tuple[int, int]]:
    """The parts of code[lo:hi] that are the type's own scope: not the body of a nested named type,
    or of a struct or union that declares an instance (the members of an anonymous one are the
    type's)."""
    spans, cur, i = [], lo, lo
    while (j := code.find("{", i, hi)) >= 0:
        k = max(code.rfind(c, lo, j) for c in ";{}")
        prefix = code[k + 1 if k >= 0 else lo:j]
        close = matching(code, j)
        anonymous = re.fullmatch(r"\s*(?:struct|union)\s*", prefix) and code[close + 1:close + 40].lstrip()[:1] == ";"
        if re.search(r"\b(?:struct|union|class|enum)\b", prefix) and "(" not in prefix and not anonymous:
            spans.append((cur, j))
            cur = i = close + 1
        else:
            i = j + 1
    spans.append((cur, hi))
    return spans


def bare(code: str, at: int) -> bool:
    """Whether the name at `at` is not the member of some other object (`x.name`, `x->name`, `X::name`)."""
    k = at - 1
    while k >= 0 and code[k] in " \t\r\n":
        k -= 1
    return k < 0 or not (code[k] == "." or code[max(k - 1, 0):k + 1] in ("->", "::"))


def declarations(text: str) -> dict[str, set[str]]:
    """Each name the code declares as `T name`, `T *name` or `T &name`, with its types."""
    out: dict[str, set[str]] = defaultdict(set)
    for m in DECL.finditer(QUALIFIERS.sub(" ", text)):
        if m.group(1) not in NOT_TYPES:
            out[m.group(2)].add(m.group(1))
    return out


class Scoped:
    """The rename of one member of one type, and what it could not prove."""

    def __init__(self, typ: str, old: str, new: str):
        self.typ, self.old, self.new = typ, old, new
        self.counts = {"definition": 0, "offsetof": 0, "access": 0}
        self.warnings: dict[str, list] = {}

    @staticmethod
    def definitions(typ: str, tree: Tree) -> list[tuple[Path, int, int, list[tuple[int, int]]]]:
        pattern = re.compile(rf"\b(?:struct|class|union)\s+{typ}\b\s*(?::[^{{;]*)?\{{")
        out = []
        for p, code in tree.codes.items():
            if typ in tree.tokens[p]:
                for m in pattern.finditer(code):
                    close = matching(code, m.end() - 1)
                    out.append((p, m.end() - 1, close, own_spans(code, m.end(), close)))
        return out

    def occurrences(self, code: str, spans: list[tuple[int, int]], name: str) -> list[int]:
        word = re.compile(rf"(?<!{WORD}){name}(?!{WORD})")
        return [lo + m.start() for lo, hi in spans for m in word.finditer(code[lo:hi]) if bare(code, lo + m.start())]

    def refusals(self, tree: Tree) -> list[str]:
        if not IDENT.match(self.new) or self.new in KEYWORDS:
            return [f"{self.new!r} is not a name C++ allows"]
        defs = self.definitions(self.typ, tree)
        if not defs:
            return [f"{self.typ} has no struct, class or union definition in the tree"]
        if not any(self.occurrences(tree.codes[p], spans, self.old) for p, _, _, spans in defs):
            return [f"{self.typ} does not declare {self.old}"]
        if any(self.occurrences(tree.codes[p], spans, self.new) for p, _, _, spans in defs):
            return [f"{self.new} is already a member of {self.typ}"]
        return []

    def edits(self, tree: Tree) -> dict[Path, dict[int, str]]:
        """Where the member is renamed, by file: start offset -> category."""
        typ, old = self.typ, self.old
        found: dict[Path, dict[int, str]] = defaultdict(dict)
        mention = re.compile(rf"(?<!{WORD}){old}(?!{WORD})")
        files = [p for p, c in tree.codes.items() if old in tree.tokens[p]]
        method = re.compile(rf"(?<!{WORD}){typ}::~?\w+\s*\(")
        class_units: dict[Path, set[int]] = defaultdict(set)
        for p, opened, close, spans in self.definitions(typ, tree):
            for at in self.occurrences(tree.codes[p], spans, old):
                found[p][at] = "definition"
            class_units[p].add(opened)
        global_types: dict[str, set[str]] = defaultdict(set)
        for p, code in tree.codes.items():
            if typ in tree.tokens[p]:
                blank = list(code)
                for a, b, opened in top_level(code):
                    if opened >= 0:
                        blank[opened:b] = " " * (b - opened)
                flat = "".join(blank)
                while (shrunk := re.sub(r"\([^()]*\)", " ", flat)) != flat:
                    flat = shrunk
                for name, types in declarations(flat).items():
                    global_types[name] |= types
        global_roots = {n for n, t in global_types.items() if t == {typ}}
        for p in files:
            code = tree.codes[p]
            for a, b, opened in top_level(code):
                text = code[a:b]
                if not mention.search(text):
                    continue
                roots = set(global_roots)
                for name, types in declarations(text).items():
                    roots.discard(name)
                    if types == {typ}:
                        roots.add(name)
                if opened in class_units[p] or (opened >= 0 and method.search(code[a:opened])):
                    roots.add("this")
                    if opened not in class_units[p]:
                        for at in self.occurrences(code, own_spans(code, opened + 1, b - 1), old):
                            found[p][at] = "definition"
                if roots:
                    names = "|".join(re.escape(r) for r in sorted(roots, key=len, reverse=True))
                    access = re.compile(rf"(?<![\w.])(?<!->)(?:{names})\s*(?:->|\.)\s*({old})(?!{WORD})")
                    for m in access.finditer(text):
                        found[p].setdefault(a + m.start(1), "access")
            for pattern in (rf"\(\s*(?:struct\s+)?{typ}\s*\*\s*\)\s*\w+\s*\)\s*->\s*({old})(?!{WORD})",
                            rf"<\s*(?:struct\s+)?{typ}\s*\*\s*>\s*\([^()]*\)\s*->\s*({old})(?!{WORD})"):
                for m in re.finditer(pattern, code):
                    found[p].setdefault(m.start(1), "access")
            for m in re.finditer(rf"\boffsetof\s*\(\s*(?:struct\s+)?{typ}\s*,\s*({old})\s*\)", code):
                found[p][m.start(1)] = "offsetof"
                text = tree.texts[p]
                message = re.compile(rf'[^;"]*?,\s*"({old})"').match(text, m.end())
                if message:
                    found[p][message.start(1)] = "offsetof"
            access_any = re.compile(rf"([A-Za-z_]\w*|\)|\])\s*(->|\.)\s*({old})(?!{WORD})")
            for m in access_any.finditer(code):
                if m.start(3) not in found[p]:
                    prev = "(...)" if m.group(1) == ")" else "[...]" if m.group(1) == "]" else m.group(1)
                    where = f"{p.relative_to(ROOT)}:{tree.texts[p].count(chr(10), 0, m.start(3)) + 1}"
                    self.warnings.setdefault(f"{prev}{m.group(2)}{old}", []).append(where)
        return found

    def apply_source(self, texts: dict[Path, str], tree: Tree) -> dict[Path, str]:
        out = {}
        for p, at in self.edits(tree).items():
            text = texts[p]
            for start in sorted(at, reverse=True):
                text = text[:start] + self.new + text[start + len(self.old):]
            for kind in at.values():
                self.counts[kind] += 1
            out[p] = text
        return out

    def data_text(self, text: str) -> str:
        qualified = re.compile(rf"(?<!{WORD}){self.typ}(::|\.){self.old}(?!{WORD})")
        return qualified.sub(lambda m: f"{self.typ}{m.group(1)}{self.new}", text)

    def apply_data(self, dry_run: bool) -> list[str]:
        changed = []
        kind = KIND_OF.get(self.typ)

        def row_edit(key: str, column: str, value: str):
            def edit(header: list[str], row: list[str]) -> bool:
                if key in header and column in header and header.index(column) < len(row) \
                        and row[header.index(key)] == value and row[header.index(column)] == self.old:
                    row[header.index(column)] = self.new
                    return True
                return False
            return edit

        for path in sorted(ROOT.glob("data/*.csv")):
            name = str(path.relative_to(ROOT))
            if path.stem in GENERATED:
                continue
            before = path.read_bytes().decode("latin-1")
            after = self.data_text(before)
            if name in ROW_KEYED:
                key, column = ROW_KEYED[name]
                after = edit_cells(after, row_edit(key, column, self.typ))
            if name in KIND_KEYED and kind:
                after = edit_cells(after, row_edit("kind", "name", kind))
            if after != before:
                self.counts["data"] = self.counts.get("data", 0) + 1
                changed.append(name)
                if not dry_run:
                    path.write_bytes(after.encode("latin-1"))
        for pattern in TEXT_FILES:
            for path in sorted(ROOT.glob(pattern)):
                before = path.read_bytes().decode("latin-1")
                after = self.data_text(before)
                if after != before:
                    changed.append(str(path.relative_to(ROOT)))
                    if not dry_run:
                        path.write_bytes(after.encode("latin-1"))
        return changed

    def report(self) -> list[str]:
        lines = [f"{self.typ}::{self.old} -> {self.new}: {self.counts['definition']} definition, "
                 f"{self.counts['offsetof']} offsetof, {self.counts['access']} access site(s), "
                 f"{self.counts.get('data', 0)} data file(s)"]
        if self.warnings:
            total = sum(len(v) for v in self.warnings.values())
            lines.append(f"warning: {total} other spelling(s) of {self.old} could not be proven to be a "
                         f"{self.typ}; check them:")
            for key, where in sorted(self.warnings.items(), key=lambda kv: -len(kv[1]))[:WARN_LIMIT]:
                lines.append(f"  {key}: {len(where)} site(s), first {where[0]}")
            if len(self.warnings) > WARN_LIMIT:
                lines.append(f"  ... and {len(self.warnings) - WARN_LIMIT} more expression(s)")
        return lines


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
    ap.add_argument("names", nargs="*", help="OLD NEW pairs (OLD may be Type::member)")
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
    scoped = [(o, n) for o, n in pairs if "::" in o]
    pairs = [pr for pr in pairs if "::" not in pr[0]]
    for old, _ in scoped:
        if not SCOPED_NAME.match(old):
            ap.error(f"{old!r} is not Type::member")
    texts = {p: p.read_bytes().decode("latin-1") for p in sources_and_headers()}
    tree = Tree(texts)
    runs = [Scoped(*SCOPED_NAME.match(o).groups(), n) for o, n in scoped]
    bad = {(r.typ + "::" + r.old, r.new): why for r in runs if (why := r.refusals(tree))}
    if bad:
        print(f"refused {len(bad)} of {len(runs)} scoped pair(s):")
        for (old, new), why in sorted(bad.items()):
            print(f"  {old} -> {new}: {'; '.join(why)}")
        if not args.keep_going:
            sys.exit(2)
        runs = [r for r in runs if (r.typ + "::" + r.old, r.new) not in bad]
        if not runs and not pairs:
            sys.exit(2)
    changed: list[str] = []
    while pairs:
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
        gone = {base_of[pr] for pr in refused}
        pairs = [p for p in pairs if p not in gone]
        if not pairs and not runs:
            sys.exit(2)
    if pairs:
        pairs += list(extra)
        changed = apply(pairs, texts, args.dry_run, docs=not args.join)
        if runs and not args.dry_run:
            texts = {p: p.read_bytes().decode("latin-1") for p in sources_and_headers()}
            tree = Tree(texts)
    reports = []
    original = dict(texts)
    for i, run in enumerate(runs):
        texts.update(run.apply_source(texts, tree))
        changed += run.apply_data(args.dry_run)
        reports += run.report()
        if i + 1 < len(runs):
            tree = Tree(texts)
    for path, text in texts.items():
        if text != original[path]:
            changed.append(str(path.relative_to(ROOT)))
            if not args.dry_run:
                path.write_bytes(text.encode("latin-1"))
    changed = list(dict.fromkeys(changed))
    for f in changed:
        print(f)
    for line in reports:
        print(line)
    print(f"{len(pairs) + len(runs)} pair(s): {'would change' if args.dry_run else 'changed'} {len(changed)} file(s)")
    if changed and not args.dry_run:
        print("Regenerate, in this order, then run the --check of each:")
        for cmd in FOLLOW_UP:
            print(f"  {cmd}")


if __name__ == "__main__":
    main()
