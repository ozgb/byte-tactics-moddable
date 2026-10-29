"""Declare each function and global once, in the header of the file that defines it.

    uv run tools/headers.py                   # write the headers and the includes
    uv run tools/headers.py --check           # exit 1 if a .cpp declares what it does not define, or a write would change a file
    uv run tools/headers.py --dry-run         # print what a write would do, change nothing
    uv run tools/headers.py --report FILE     # as --dry-run, into FILE

The defining file of a function is the file under src/ with a definition of
it (the `// FUNCTION:` marker that tools/sources.py reads sits above each one),
and of a global the file under src/ or link/ that defines it (a `// GLOBAL:`
marker names it in a data file). Its declaration goes in that file's header:
foo.cpp gets foo.h next to it, created when missing, and a global defined in
link/data.cpp is declared by link/globals.h already. Every other .cpp loses
its hand-written prototype or extern of the name, and gets the #include of the
header at the place the first declaration was.

A declaration is written from the definition: its return type, convention (the
effective one, spelled out), __declspec and parameter types, with the default
arguments dropped, and the parameter names kept. Types it names are
forward-declared when they are structs, classes or unions, and the header that
defines them is included for typedefs, enums and templates. A function that
names a type only its own .cpp defines is left alone and reported.

Not touched: static functions and variables, members of classes (declared in
their class), anything in an anonymous namespace, inline functions, templates,
declarations inside a function and the externs a `// GLOBAL:` marker sits on.

A prototype or extern that disagrees with the definition is reported as a
mismatch (file, declaration, definition). The definition wins, so the include
brings the definition's type; a name is overloaded when several definitions
share it and the one with the same parameters is chosen, otherwise it is
reported as ambiguous and left alone.

A declaration already in the file's own header is compared with the definition too (return
type, convention, parameter types, linkage, qualifier) and rewritten from it when they
differ, with its default arguments kept; --check fails until it is. The names of a
data/headers_left_out.csv row are not rewritten, and a rewrite that would name a placeholder
type is a skip.

The names of its parameters are kept, except placeholder ones (param_N), which are
dropped. A function or global whose declaration would name a type with a placeholder
name (<Name>_<addr>) is not declared: it is reported as a skip and its prototypes stay,
so that the placeholder names are not spread before the types are named. A .cpp that
uses none of the names it lost does not get the include.

--check passes the declarations that data/headers_left_out.csv lists (columns
name, kind, reason; kind is mismatch, ambiguous or skip) and fails on any other,
and on a listed name that no longer needs listing.

Each .cpp that has a header next to it includes it, after its other includes (or at the
top when it has none), unless that would define a type a second time in the file, or
data/headers_no_self_include.csv (columns path, reason) lists the file because the include
does not build or hostlint refuses it; --check fails on a listed file that already includes
its header or has none. The report names each file left out and why.

Running it twice changes nothing. Generated files, src/util/fixed_trig.cpp
(left out of the build) and src/state/layout_asserts.cpp are skipped.
Standard library only.
"""

import argparse
import re
import sys
from dataclasses import dataclass, field
from pathlib import Path

import commentlint
import sources

ROOT = sources.ROOT
SKIP_FILES = {"src/harness/trace_schema.h", "src/state/layout_asserts.cpp", "src/util/fixed_trig.cpp"}
LINK_DATA = "link/data.cpp"
LINK_HEADER = "link/globals.h"
TYPE_DIRS = ("src", "include", "interfaces", "link")
CONVENTIONS = ("__cdecl", "__stdcall", "__fastcall", "__thiscall")
DEFAULT_CONVENTION = "__stdcall"
BUILTIN = {
    "void", "char", "short", "int", "long", "float", "double", "unsigned", "signed", "bool", "wchar_t",
    "__int8", "__int16", "__int32", "__int64", "const", "volatile", "struct", "class", "union", "enum",
    "static", "extern", "inline", "register", "typename", "operator", "sizeof", "__forceinline",
    "__declspec", "dllimport", "dllexport", "naked", "noreturn", "align", "thread", "new", "delete",
    "true", "false", "this", "__w64", "__ptr64", "__restrict", "__unaligned",
} | set(CONVENTIONS)
TAGS = ("struct", "class", "union")
PLACEHOLDER_PARAM = re.compile(r"^param_[0-9]+$")
PLACEHOLDER_TYPE = re.compile(r"^[A-Za-z]\w*_00[0-9a-fA-F]{6}$")
LEFT_OUT = "data/headers_left_out.csv"
NO_SELF_INCLUDE = "data/headers_no_self_include.csv"


@dataclass
class Item:
    """A top-level declaration or definition in a file."""
    kind: str
    begin: int
    end: int
    name: str = ""
    text: str = ""
    prefix: str = ""
    params: str = ""
    post: str = ""
    extents: str = ""
    storage: frozenset = frozenset()
    scope: str = ""
    anon: bool = False
    block: bool = False
    has_init: bool = False
    tag: str = ""
    defined: bool = False
    marker: int | None = None
    marker_line: tuple[int, int] | None = None
    line: int = 0


@dataclass
class Source:
    path: str
    text: str
    masked: str
    items: list
    includes: list
    defines: set


def mask_source(text):
    """The text with comments and the insides of strings blanked (offsets and
    newlines kept), and preprocessor lines blanked; plus the #include lines
    and the names #define'd."""
    chars = list(text)
    for c in commentlint.scan_c(text):
        for k in range(c.begin, c.stop):
            if chars[k] != "\n":
                chars[k] = " "
    out = "".join(chars)

    def blank(m):
        return m.group(0)[0] + re.sub(r"[^\n]", " ", m.group(0)[1:-1]) + m.group(0)[-1]

    out = re.sub(r'"(?:\\.|[^"\\\n])*"', blank, out)
    out = re.sub(r"'(?:\\.|[^'\\\n])*'", blank, out)
    includes, defines = [], set()
    lines = out.split("\n")
    pos = 0
    i = 0
    while i < len(lines):
        line = lines[i]
        if line.lstrip().startswith("#"):
            j = i
            while lines[j].rstrip().endswith("\\") and j + 1 < len(lines):
                j += 1
            directive = " ".join(l.strip() for l in lines[i:j + 1])
            m = re.match(r"#\s*include\s*([<\"])([^>\"]*)[>\"]", text[pos:pos + len("\n".join(lines[i:j + 1]))].lstrip())
            if m:
                includes.append((pos, m.group(1), m.group(2)))
            md = re.match(r"#\s*define\s+(\w+)", directive)
            if md:
                defines.add(md.group(1))
            for k in range(i, j + 1):
                lines[k] = re.sub(r"[^\n]", " ", lines[k])
            pos += sum(len(l) + 1 for l in lines[i:j + 1])
            i = j + 1
            continue
        pos += len(line) + 1
        i += 1
    return "\n".join(lines), includes, defines


def match_brace(m, i):
    """Index of the } matching the { at i."""
    depth = 0
    for k in range(i, len(m)):
        if m[k] == "{":
            depth += 1
        elif m[k] == "}":
            depth -= 1
            if depth == 0:
                return k
    return len(m) - 1


def top_level(s, ch):
    """Index of the first ch in s outside brackets, or -1."""
    depth = 0
    for k, c in enumerate(s):
        if c == ch and depth == 0:
            return k
        if c in "([{":
            depth += 1
        elif c in ")]}":
            depth -= 1
    return -1


def has_top_level_equals(head):
    head = re.sub(r"operator\s*[^\s\w(]+", "operator", head)
    depth = 0
    for k, c in enumerate(head):
        if c in "([{":
            depth += 1
        elif c in ")]}":
            depth -= 1
        elif c == "=" and depth == 0 and head[k - 1:k] not in "=!<>" and head[k + 1:k + 2] != "=":
            return True
    return False


def split_statements(masked, text):
    """(begin, end, scope names, anonymous, in_block, has_body) of each top-level statement."""
    n = len(masked)
    out = []
    scopes = []
    start = 0
    i = 0
    pdepth = 0
    while i < n:
        c = masked[i]
        if c in "([":
            pdepth += 1
        elif c in ")]":
            pdepth = max(0, pdepth - 1)
        elif c == ";" and pdepth == 0:
            out.append((start, i + 1, tuple(scopes), False))
            start = i + 1
        elif c == "{" and pdepth == 0:
            head = masked[start:i]
            stripped = head.strip()
            mns = re.match(r"namespace\b\s*(\w*)\s*$", stripped)
            if mns or re.match(r"extern\s*$", re.sub(r'"[^"]*"', "", text[start:i].strip())) and '"' in text[start:i]:
                scopes.append(("ns:" + mns.group(1)) if mns else "extern_c")
                start = i + 1
            else:
                j = match_brace(masked, i)
                is_type = re.search(r"\b(struct|class|union|enum)\b", stripped) and "(" not in stripped
                is_init = has_top_level_equals(head)
                ctor_init = re.search(r"\)\s*:", head) and re.search(r"[\w>]\s*$", head)
                if is_type or is_init or ctor_init:
                    i = j
                else:
                    out.append((start, j + 1, tuple(scopes), True))
                    start = j + 1
                    i = j
        elif c == "}" and pdepth == 0:
            if scopes:
                scopes.pop()
            start = i + 1
        i += 1
    return out


FNPTR = re.compile(r"^(?P<type>.*?)\(\s*(?:(?P<cc>__\w+)\s+)?\*\s*(?P<name>\w+)\s*(?P<arr>(?:\[[^\]]*\]\s*)*)\)\s*\((?P<params>.*)\)\s*$", re.S)
NAME_TAIL = re.compile(r"(operator\s*(?:\(\)|\[\]|[^\s(\w]+|\w+)|~?\w+(?:\s*::\s*~?\w+)*)\s*$")


def collapse_bodies(s):
    """The statement with each {...} body replaced by {}."""
    out, depth, k = [], 0, 0
    while k < len(s):
        c = s[k]
        if c == "{":
            if depth == 0:
                out.append("{}")
            depth += 1
        elif c == "}":
            depth -= 1
        elif depth == 0:
            out.append(c)
        k += 1
    return "".join(out)


def parse_statement(text, masked, begin, end, scopes, has_body):
    """The Item for one top-level statement."""
    raw = masked[begin:end]
    stmt = collapse_bodies(raw)
    t = stmt.strip().rstrip(";").strip()
    anon = "ns:" in "".join(scopes) and any(s == "ns:" for s in scopes)
    scope = "::".join(s[3:] for s in scopes if s.startswith("ns:") and s != "ns:")
    block = "extern_c" in scopes
    orig = text[begin:end]
    item = Item("other", begin, end, text=t, scope=scope, anon=anon, block=block)
    if not t:
        return item
    storage = set()
    m = re.match(r'extern\s*"\s*"\s*', t)
    if m:
        storage.update(("extern_c", "extern"))
        t = t[m.end():]
    while True:
        m = re.match(r"(extern|static|inline|__inline|__forceinline|register)\b\s*", t)
        if not m:
            break
        storage.add(m.group(1))
        t = t[m.end():]
    if "inline" in storage or "__inline" in storage or "__forceinline" in storage:
        storage.add("inline")
    item.storage = frozenset(storage)
    if re.match(r"(typedef|using|template|friend|namespace|static_assert|asm|__asm)\b", t):
        item.kind = "template" if t.startswith("template") else "other"
        return item
    mt = re.match(r"(struct|class|union|enum)\b\s*(\w*)\s*(?P<rest>[:{;]|$)", t)
    if mt and (mt.group("rest") in (":", "{", ";", "") or not mt.group(2)):
        item.kind = "type"
        item.tag = mt.group(1)
        item.name = mt.group(2)
        item.defined = "{" in raw
        return item
    has_paren = top_level(t, "(") >= 0
    mf = FNPTR.match(t.split("=", 1)[0].strip()) if has_paren else None
    if mf:
        item.kind = "var_def" if ("extern" not in storage or has_top_level_equals(t)) else "var_decl"
        item.name = mf.group("name")
        item.prefix = t
        item.has_init = has_top_level_equals(t)
        return item
    if has_paren:
        pos = top_level(t, "(")
        before = t[:pos]
        mn = NAME_TAIL.search(before)
        if not mn or not before[:mn.start()].strip():
            return item
        depth, close = 0, -1
        for k in range(pos, len(t)):
            if t[k] == "(":
                depth += 1
            elif t[k] == ")":
                depth -= 1
                if depth == 0:
                    close = k
                    break
        if close < 0:
            return item
        item.name = re.sub(r"\s+", "", mn.group(1)) if "operator" not in mn.group(1) else "operator"
        item.prefix = before[:mn.start()].strip()
        item.params = t[pos + 1:close]
        item.post = t[close + 1:].replace("{}", "").strip()
        item.kind = "func_def" if has_body else "func_proto"
        if has_body is False and item.post.startswith(("=", ":")):
            item.kind = "other"
        return item
    if has_body:
        return item
    decl = t.split("=", 1)[0]
    item.has_init = has_top_level_equals(t)
    if top_level(decl, ",") >= 0:
        item.kind = "other"
        return item
    mv = re.search(r"(\w+)\s*((?:\[[^\]]*\]\s*)*)$", decl.strip())
    if not mv or mv.group(1) in BUILTIN:
        return item
    item.name = mv.group(1)
    item.extents = re.sub(r"\s+", "", mv.group(2))
    item.prefix = decl.strip()[:mv.start()].strip()
    if not item.prefix:
        item.kind = "other"
        return item
    item.kind = "var_decl" if "extern" in storage and not item.has_init else "var_def"
    return item


def line_of(text, offset):
    return text.count("\n", 0, offset) + 1


def marker_before(text, begin):
    """The address of the // FUNCTION: or // GLOBAL: marker line directly above an offset, and its span."""
    lines_before = text[:begin].split("\n")
    k = len(lines_before) - 2
    found = None
    while k >= 0:
        s = lines_before[k].strip()
        m = sources.ANNOTATION.match(s) or sources.GLOBAL_ANNOTATION.match(s)
        if m:
            found = (int(m.group(1), 16), k)
            k -= 1
            continue
        break
    if not found:
        return None, None
    first = k + 1
    b = sum(len(l) + 1 for l in lines_before[:first])
    return found[0], (b, begin)


def parse_source(path, text):
    masked, includes, defines = mask_source(text)
    items = []
    for begin, end, scopes, has_body in split_statements(masked, text):
        s = begin
        while s < end and masked[s].isspace():
            s += 1
        item = parse_statement(text, masked, s, end, scopes, has_body)
        item.line = line_of(text, s)
        if item.kind in ("func_def", "var_def", "var_decl", "func_proto"):
            item.marker, item.marker_line = marker_before(text, s)
        items.append(item)
    return Source(path, text, masked, items, includes, defines)


def read(path):
    return (ROOT / path).read_bytes().decode("utf-8", errors="surrogateescape")


def project_files(suffixes, dirs=TYPE_DIRS):
    out = []
    for d in dirs:
        out += [str(p.relative_to(ROOT)) for p in (ROOT / d).rglob("*") if p.suffix in suffixes]
    return sorted(set(out) - SKIP_FILES)



def split_commas(s):
    """The pieces of s between its top-level commas."""
    out, depth, cur = [], 0, []
    for k, c in enumerate(s):
        if c in "([{<" and not (c == "<" and s[k - 1:k] in ("<", " ") and s[k + 1:k + 2] in ("=", " ")):
            depth += 1
        elif c in ")]}>" and not (c == ">" and s[k - 1:k] == "-"):
            depth = max(0, depth - 1)
        if c == "," and depth == 0:
            out.append("".join(cur))
            cur = []
        else:
            cur.append(c)
    out.append("".join(cur))
    return [p.strip() for p in out]


QUALIFIERS = {"const", "volatile", "struct", "class", "union", "enum", "register", "__unaligned", "__restrict"}


@dataclass
class Param:
    type: str
    name: str
    default: str | None
    text: str


def parse_param(p):
    """The type, the name and the default of one parameter."""
    p = p.strip()
    default = None
    k = top_level(p, "=")
    if k >= 0:
        default = p[k + 1:].strip()
        p = p[:k].strip()
    if p in ("", "void", "..."):
        return Param(p, "", default, p)
    m = re.search(r"\(\s*(?:__\w+\s+)?[*&]+\s*(\w+)\s*(?:\[[^\]]*\]\s*)*\)", p)
    if m:
        return Param(p[:m.start(1)] + p[m.end(1):], m.group(1), default, p)
    arr = re.search(r"((?:\[[^\]]*\]\s*)+)$", p)
    tail = ""
    body = p
    if arr:
        tail = arr.group(1)
        body = p[:arr.start()].strip()
    m = re.search(r"([A-Za-z_]\w*)\s*$", body)
    if m and m.group(1) not in BUILTIN:
        head = body[:m.start()].strip()
        words = re.findall(r"[A-Za-z_]\w*", head)
        if head and any(w not in QUALIFIERS for w in words) or (head and head[-1] in "*&>"):
            return Param(head + tail, m.group(1), default, p)
        if arr:
            return Param(body + tail, "", default, p)
    return Param(body + tail, "", default, p)


def split_prefix(prefix):
    """(return type with __declspec, convention or "") of the text before a function name."""
    conv = ""
    toks = []
    for t in re.split(r"(\s+)", prefix):
        if t in CONVENTIONS:
            conv = t
        else:
            toks.append(t)
    ret = re.sub(r"\s+", " ", "".join(toks)).strip()
    return ret, conv


SIMPLE = [
    (r"\bunsigned\s+int\b", "unsigned"), (r"\bsigned\s+int\b", "int"), (r"\bunsigned\s+long\s+int\b", "unsigned long"),
    (r"\blong\s+int\b", "long"), (r"\bunsigned\s+short\s+int\b", "unsigned short"), (r"\bshort\s+int\b", "short"),
    (r"\b(struct|class|union|enum)\s+", ""), (r"\bextern\b|\bstatic\b", ""),
]


def norm_type(t, aliases=None, depth=0):
    """A type spelled so that equal types compare equal: tags dropped, typedefs expanded, array extents ignored."""
    t = re.sub(r"\[[^\]]*\]", "[]", t)
    if aliases and depth < 6:
        t = re.sub(r"[A-Za-z_]\w*", lambda m: "(" + aliases[m.group(0)] + ")" if m.group(0) in aliases else m.group(0), t)
        if "(" in t:
            t = re.sub(r"\(([^()]*)\)", lambda m: " " + norm_type(m.group(1), aliases, depth + 1) + " " if re.fullmatch(r"[\w\s*&]+", m.group(1)) else m.group(0), t)
    for pat, rep in SIMPLE:
        t = re.sub(pat, rep, t)
    t = re.sub(r"\bconst\b", " const ", t)
    t = re.sub(r"\s+", " ", t).strip()
    t = re.sub(r"\s*([*&\[\]<>,()])\s*", r"\1", t)
    parts = t.split(" ")
    if "const" in parts and parts[0] != "const" and not re.search(r"[*&]", t):
        parts.remove("const")
        parts.insert(0, "const")
    return " ".join(parts)



@dataclass
class Def:
    path: str
    item: Item


@dataclass
class TypeDef:
    path: str
    kind: str
    defined: bool


@dataclass
class Decl:
    """A hand-written prototype or extern in a .cpp."""
    path: str
    item: Item
    kind: str
    name: str
    status: str = "ok"
    defn: Def | None = None
    home: str = ""
    diffs: list = field(default_factory=list)


def ident_tokens(text):
    return [t for t in re.findall(r"[A-Za-z_]\w*(?:\s*::\s*\w+)*", text)]


def system_header(name):
    """The system header that defines a type the repository does not."""
    if name in ("size_t", "ptrdiff_t"):
        return "<stddef.h>"
    if re.fullmatch(r"LP\w+|H[A-Z]+|DWORD|WORD|BOOL|BYTE|UINT|ULONG|HRESULT|WPARAM|LPARAM|LRESULT", name):
        return "<windows.h>"
    return ""


def include_line(frm, target, relative):
    return f"#include {target}" if target.startswith("<") else f'#include "{relative(frm, target)}"'


def effective_convention(conv, params, default=DEFAULT_CONVENTION):
    if conv:
        return conv
    return "__cdecl" if re.search(r"\.\.\.\s*$", params.strip()) else default


class World:
    """Every source file under the root, parsed, with the indexes the planner reads."""

    def __init__(self, root):
        self.root = Path(root)
        self.files = {}
        self.cpps = []
        paths = []
        for d in TYPE_DIRS:
            if (self.root / d).is_dir():
                paths += [str(p.relative_to(self.root)) for p in (self.root / d).rglob("*") if p.suffix in (".cpp", ".h")]
        for p in sorted(set(paths)):
            self.files[p] = parse_source(p, self.read(p))
        self.cpps = [p for p in self.files if p.endswith(".cpp") and self.editable(p)]
        self.funcdefs = {}
        self.vardefs = {}
        self.hdecls = {}
        self.types = {}
        self.macros = {}
        self.aliases = {}
        self.closure_cache = {}
        self.no_self_include = load_no_self_include(root)
        self.index()

    def read(self, path):
        return (self.root / path).read_bytes().decode("utf-8", errors="surrogateescape")

    def editable(self, path):
        return path not in SKIP_FILES and not commentlint.is_generated(path, self.files[path].text if path in self.files else self.read(path))

    def home_of_cpp(self, path):
        return LINK_HEADER if path == LINK_DATA else str(Path(path).with_suffix(".h"))

    def index(self):
        for path, src in self.files.items():
            for name in src.defines:
                self.macros.setdefault(name, []).append(path)
            for it in src.items:
                if it.kind == "type" and it.name:
                    self.types.setdefault(it.name, []).append(TypeDef(path, it.tag, it.defined))
                elif it.kind == "template":
                    m = re.search(r"\b(?:struct|class)\s+(\w+)", it.text)
                    if m:
                        self.types.setdefault(m.group(1), []).append(TypeDef(path, "template", True))
                elif it.kind == "other" and it.text.startswith(("typedef", "using")):
                    self.index_typedef(path, it)
                elif path.endswith(".h") and not it.scope:
                    if it.kind == "func_proto" and "::" not in it.name:
                        self.hdecls.setdefault(it.name, []).append(Def(path, it))
                    elif it.kind == "var_decl":
                        self.hdecls.setdefault(it.name, []).append(Def(path, it))
                if path.endswith(".cpp") and path in SKIP_FILES:
                    continue
                if it.kind == "func_def" and path.endswith(".cpp") and self.exported_func(it):
                    self.funcdefs.setdefault(it.name, []).append(Def(path, it))
                elif it.kind == "var_def" and path.endswith(".cpp") and self.exported_var(it, path):
                    self.vardefs.setdefault(it.name, []).append(Def(path, it))

    def index_typedef(self, path, it):
        t = it.text
        m = re.match(r"using\s+(\w+)\s*=\s*(.*)$", t, re.S)
        if m:
            self.types.setdefault(m.group(1), []).append(TypeDef(path, "using", True))
            return
        body = t[len("typedef"):].strip()
        mf = re.search(r"\(\s*(?:\w+\s+)?[*&]?\s*(\w+)\s*\)\s*\(", body)
        mm = re.search(r"(\w+)\s*(?:\[[^\]]*\]\s*)*$", body)
        name = mf.group(1) if mf else (mm.group(1) if mm else "")
        if not name:
            return
        self.types.setdefault(name, []).append(TypeDef(path, "typedef", True))
        if not mf and mm and "{" not in body and "," not in body:
            target = body[:mm.start()].strip()
            if target and target != name:
                self.aliases.setdefault(name, target)

    @staticmethod
    def exported_func(it):
        return (it.prefix and not it.scope and not it.anon and "::" not in it.name and "static" not in it.storage
                and "inline" not in it.storage and it.name != "operator" and "operator" not in it.name
                and not it.prefix.startswith("template"))

    @staticmethod
    def exported_var(it, path):
        if it.scope or it.anon or "static" in it.storage:
            return False
        if re.search(r"\bconst\b", it.prefix) and "extern" not in it.storage and path != LINK_DATA:
            return False
        return True

    def closure(self, path):
        """The repository files a file includes, directly or not, nearest first."""
        if path in self.closure_cache:
            return self.closure_cache[path]
        seen, order, queue = {path}, [], [path]
        while queue:
            cur = queue.pop(0)
            for _, kind, name in (self.files[cur].includes if cur in self.files else []):
                target = self.resolve_include(cur, kind, name)
                if target and target not in seen:
                    seen.add(target)
                    order.append(target)
                    queue.append(target)
        self.closure_cache[path] = order
        return order

    def resolve_include(self, frm, kind, name):
        if kind == "<":
            return None
        for base in (Path(frm).parent, Path("include"), Path("interfaces"), Path("src")):
            cand = (self.root / base / name)
            if cand.is_file():
                rel = str(Path(*Path(str(cand.resolve().relative_to(self.root.resolve()))).parts))
                return rel if rel in self.files else None
        return None


    def default_convention(self, path):
        flags = re.search(r"^//\s*FLAGS:(.*)$", self.files[path].text, re.M)
        text = flags.group(1) if flags else ""
        for flag, conv in (("/Gd", "__cdecl"), ("/Gr", "__fastcall"), ("/Gz", "__stdcall")):
            if flag in text.split():
                return conv
        return DEFAULT_CONVENTION

    def func_signature(self, path, item):
        """(return type, convention, parameters, post, linkage) of a function item, normalised."""
        ret, conv = split_prefix(item.prefix)
        params = [parse_param(p) for p in split_commas(item.params)] if item.params.strip() else []
        if len(params) == 1 and params[0].type == "void":
            params = []
        eff = effective_convention(conv, item.params, self.default_convention(path))
        c = "extern_c" in item.storage or item.block
        return ret, eff, params, item.post, c

    def func_diffs(self, decl_path, decl, def_path, defn):
        a = self.func_signature(decl_path, decl)
        b = self.func_signature(def_path, defn)
        diffs = []
        if norm_type(a[0], self.aliases) != norm_type(b[0], self.aliases):
            diffs.append(f"return type {a[0]} vs {b[0]}")
        if a[1] != b[1]:
            diffs.append(f"convention {a[1]} vs {b[1]}")
        if a[4] != b[4]:
            diffs.append("linkage")
        if a[3] != b[3]:
            diffs.append(f"qualifier {a[3]!r} vs {b[3]!r}")
        pa = [norm_type(p.type, self.aliases) for p in a[2]]
        pb = [norm_type(p.type, self.aliases) for p in b[2]]
        if len(pa) != len(pb):
            diffs.append(f"{len(pa)} parameters vs {len(pb)}")
        else:
            for k, (x, y) in enumerate(zip(pa, pb)):
                if x != y:
                    diffs.append(f"parameter {k + 1} {a[2][k].type} vs {b[2][k].type}")
        return diffs

    def params_equal(self, decl_path, decl, def_path, defn):
        a = self.func_signature(decl_path, decl)[2]
        b = self.func_signature(def_path, defn)[2]
        return [norm_type(p.type, self.aliases) for p in a] == [norm_type(p.type, self.aliases) for p in b]

    def var_type(self, item):
        return norm_type(re.sub(r"\bextern\b", "", item.prefix), self.aliases)

    def var_diffs(self, decl, defn):
        diffs = []
        if self.var_type(decl) != self.var_type(defn):
            diffs.append(f"type {decl.prefix}{decl.extents} vs {defn.prefix}{defn.extents}")
        elif bool(decl.extents) != bool(defn.extents):
            diffs.append(f"extent {decl.extents or 'none'} vs {defn.extents or 'none'}")
        elif decl.extents and defn.extents:
            ea = re.findall(r"\[([^\]]*)\]", decl.extents)
            eb = re.findall(r"\[([^\]]*)\]", defn.extents)
            if len(ea) != len(eb) or any(x.strip() != y.strip() and x.strip() and y.strip() for x, y in zip(ea, eb)):
                diffs.append(f"extent {decl.extents} vs {defn.extents}")
        return diffs

    def classify(self, path, item):
        """The Decl for a hand-written prototype or extern, or None when it is not one the tool handles."""
        if item.scope or item.block or "::" in item.name or "inline" in item.storage or item.anon:
            return None
        if item.kind == "func_proto":
            if "dllimport" in item.prefix or not item.prefix or "operator" in item.name:
                return None
            d = Decl(path, item, "func", item.name)
            cands = self.funcdefs.get(item.name, [])
            if not cands:
                d.status = "nodef"
                return d
            eq = [c for c in cands if self.params_equal(path, item, c.path, c.item)]
            if len(cands) == 1:
                d.defn = cands[0]
            elif len(eq) == 1:
                d.defn = eq[0]
            else:
                d.status = "ambiguous"
                d.diffs = [f"{len(cands)} definitions: " + ", ".join(sorted({c.path for c in cands}))]
                return d
            d.diffs = self.func_diffs(path, item, d.defn.path, d.defn.item)
            if d.diffs:
                d.status = "mismatch"
            return d
        if item.kind == "var_decl":
            d = Decl(path, item, "var", item.name)
            cands = self.vardefs.get(item.name, [])
            if not cands:
                d.status = "nodef"
                return d
            if len(cands) > 1:
                d.status = "ambiguous"
                d.diffs = [f"{len(cands)} definitions: " + ", ".join(sorted({c.path for c in cands}))]
                return d
            d.defn = cands[0]
            d.diffs = self.var_diffs(item, d.defn.item)
            if d.diffs:
                d.status = "mismatch"
            return d
        return None

    def home_for(self, d):
        """The header that declares a definition: one that already does, else the definer's own."""
        existing = [h.path for h in self.hdecls.get(d.name, [])]
        own = self.home_of_cpp(d.defn.path)
        if own in existing or not existing:
            return own
        near = [p for p in self.closure(d.defn.path) if p in existing]
        return near[0] if near else sorted(existing)[0]

    def render(self, kind, defn, defaults=None):
        """(declaration text, identifiers it names, default arguments) for a definition."""
        it = defn.item
        if kind == "func":
            ret, conv = split_prefix(it.prefix)
            eff = effective_convention(conv, it.params, self.default_convention(defn.path))
            params = [parse_param(p) for p in split_commas(it.params)] if it.params.strip() else []
            defaults = defaults or {}
            text = re.sub(r"\s+", " ", ", ".join(self.param_text(p) + (f" = {defaults[k]}" if defaults.get(k) else "")
                                                 for k, p in enumerate(params))) if params else ""
            if not params and it.params.strip() == "void":
                text = "void"
            link = 'extern "C" ' if "extern_c" in it.storage or it.block else ""
            decl = f"{link}{ret} {eff} {it.name}({text}){(' ' + it.post) if it.post else ''};"
            names = set(ident_tokens(ret))
            for p in params:
                names |= set(ident_tokens(p.type))
            return decl, names
        t = re.sub(r"\s+", " ", it.prefix)
        t = re.sub(r"\bextern\b|\bstatic\b", "", t).strip()
        if it.prefix == it.text.split("=")[0].strip() and FNPTR.match(t):
            decl = f"extern {t};"
            names = set(ident_tokens(t))
            m = FNPTR.match(t)
            names.discard(m.group("name"))
        else:
            decl = f"extern {t} {it.name}{it.extents};"
            names = set(ident_tokens(t))
        return decl, names

    @staticmethod
    def param_text(p):
        text = re.sub(r"\s+", " ", p.type).strip()
        if p.name and not PLACEHOLDER_PARAM.match(p.name):
            if "(" in p.text and re.search(r"\(\s*(?:__\w+\s+)?[*&]+\s*\)", text):
                text = re.sub(r"\((\s*(?:__\w+\s+)?[*&]+)\s*\)", lambda m: "(" + m.group(1) + p.name + ")", text, count=1)
            elif re.search(r"\[[^\]]*\]\s*$", text):
                text = re.sub(r"\s*((?:\[[^\]]*\]\s*)+)$", lambda m: " " + p.name + m.group(1), text)
            else:
                text = text + " " + p.name
        return text.replace("* ", "*").replace("& ", "&") if False else text

    def resolve_names(self, names, definer, header):
        """(forward declarations, includes, problems) a header needs for the identifiers it names."""
        fwd, inc, problems = set(), set(), []
        visible = [header] + self.closure(header) if header in self.files else [header]
        for t in sorted(names):
            if t in BUILTIN:
                continue
            if "::" in t:
                problems.append(f"qualified name {t}")
                continue
            defs = self.types.get(t, [])
            tags = [d for d in defs if d.kind in TAGS]
            others = [d for d in defs if d.kind not in TAGS and d.kind != "enum" or d.kind == "enum"]
            nontag = [d for d in defs if d.kind not in TAGS]
            if any(d.path in visible and (d.defined or d.kind in ("typedef", "using", "enum", "template")) for d in defs):
                continue
            if tags:
                chosen = sorted(tags, key=lambda d: (not d.defined, d.path))[0]
                fwd.add(f"{chosen.kind} {t};")
                continue
            if nontag:
                near = [p for p in self.closure(definer) if any(d.path == p for d in nontag)]
                if near:
                    inc.add(near[0])
                    continue
                if any(d.path == definer for d in nontag):
                    problems.append(f"{t} is defined in {definer}")
                    continue
                heads = sorted({d.path for d in nontag if d.path.endswith(".h")})
                if len(heads) == 1:
                    inc.add(heads[0])
                else:
                    problems.append(f"{t} is defined in {len(heads)} headers")
                continue
            if t in self.macros:
                near = [p for p in self.closure(definer) if p in self.macros[t]]
                if near:
                    inc.add(near[0])
                elif definer in self.macros[t]:
                    problems.append(f"macro {t} is defined in {definer}")
                else:
                    problems.append(f"macro {t} is defined in {len(self.macros[t])} files")
                continue
            system = system_header(t)
            if system:
                inc.add(system)
                continue
            problems.append(f"unknown name {t}")
        return fwd, inc, problems


    def param_spans(self, path, item):
        """Offsets, in the file, of each parameter of a function item's list."""
        masked = self.files[path].masked
        pos = item.begin
        depth = 0
        first = None
        k = pos
        while k < item.end:
            c = masked[k]
            if c == "(" and first is None and depth == 0:
                first = k
                depth = 1
            elif first is not None:
                if c in "([{<":
                    depth += 1
                elif c in ")]}>":
                    depth -= 1
                    if depth == 0:
                        break
            k += 1
        if first is None:
            return []
        end = k
        spans, cur, d = [], first + 1, 0
        for j in range(first + 1, end + 1):
            c = masked[j]
            if c in "([{<":
                d += 1
            elif c in ")]}>" and j < end:
                d -= 1
            if (c == "," and d == 0) or j == end:
                spans.append((cur, j))
                cur = j + 1
        return spans

    def defined_types(self, path):
        """The names of the types a file defines itself."""
        cache = self.__dict__.setdefault("_defined", {})
        if path not in cache:
            names = set()
            for it in self.files[path].items if path in self.files else []:
                if it.kind == "type" and it.defined and it.name:
                    names.add(it.name)
            for n, defs in self.types.items():
                if any(d.path == path and d.kind in ("typedef", "using") for d in defs):
                    names.add(n)
            cache[path] = names
        return cache[path]

    def visible_types(self, path):
        out = {}
        for p in [path] + self.closure(path):
            for n in self.defined_types(p):
                out.setdefault(n, set()).add(p)
        return out

    def type_conflict(self, cpp, header):
        """A type name a header would define a second time in a file, or None."""
        a = self.visible_types(cpp)
        b = self.visible_types(header)
        for t, sites in sorted(b.items()):
            have = a.get(t)
            if have and sites - have:
                return t
        return None

    def statics(self, path):
        cache = self.__dict__.setdefault("_statics", {})
        if path not in cache:
            names = set()
            for it in self.files[path].items:
                if it.kind in ("func_def", "func_proto", "var_def", "var_decl") and ("static" in it.storage or it.anon):
                    names.add(it.name)
            cache[path] = names
        return cache[path]

    def seers(self, header):
        cache = self.__dict__.setdefault("_seers", {})
        if header not in cache:
            cache[header] = [f for f in self.cpps if header in self.closure(f)]
        return cache[header]

    def plan(self, mismatch="skip", left_out=frozenset()):
        plan = Plan()
        decls = []
        for path in self.cpps:
            if path == LINK_DATA:
                continue
            for it in self.files[path].items:
                d = self.classify(path, it)
                if d is None:
                    continue
                if d.kind == "var" and it.marker is not None:
                    plan.kept_marker.append(d)
                    continue
                decls.append(d)
        for d in decls:
            if d.defn is None:
                (plan.nodef if d.status == "nodef" else plan.ambiguous).append(d)
                continue
            d.home = self.home_for(d)
            if d.status == "mismatch":
                plan.mismatches.append(d)
        bad = set()
        blocked = set()
        for d in decls:
            if d.home == LINK_HEADER:
                continue
            if d.status in ("mismatch", "ambiguous"):
                bad.add((d.kind, d.name))
        linkdefs = {n: v[0] for n, v in self.vardefs.items() if len(v) == 1 and v[0].path == LINK_DATA}
        for path in self.cpps:
            if path == LINK_DATA:
                continue
            for p in [path] + self.closure(path):
                for it in self.files[p].items:
                    if it.kind in ("var_decl", "var_def") and it.name in linkdefs and not it.scope and p != LINK_HEADER:
                        if self.var_diffs(it, linkdefs[it.name].item):
                            blocked.add((path, LINK_HEADER))
        for d in decls:
            if d.home == LINK_HEADER and d.status in ("mismatch", "ambiguous"):
                blocked.add((d.path, LINK_HEADER))
        for d in decls:
            if d.defn is not None and d.home and (d.path, d.home) not in blocked:
                t = self.type_conflict(d.path, d.home)
                if t:
                    blocked.add((d.path, d.home))
                    plan.type_conflicts.append((d, t))
        plan.bad_names = sorted(bad)
        demand = {}
        for d in decls:
            if d.defn is None or (d.kind, d.name) in bad or (d.path, d.home) in blocked:
                continue
            demand.setdefault(d.home, {}).setdefault(d.name, []).append(d)
        homed = {}
        for home, names in sorted(demand.items()):
            for name, ds in names.items():
                defn = ds[0].defn
                kind = ds[0].kind
                existing = [h for h in self.hdecls.get(name, []) if h.path == home]
                text, idents = self.render(kind, defn)
                if kind == "func":
                    text, idents = self.render(kind, defn, self.default_arguments(ds, defn))
                clash = [f for f in self.seers(home) if name in self.statics(f)]
                if clash:
                    for d in ds:
                        plan.skipped.append((d, f"{clash[0]} defines a static {name} and sees {home}"))
                    continue
                if existing:
                    homed[(home, name)] = (kind, defn, None, set(), set())
                    continue
                held = sorted(i for i in idents if PLACEHOLDER_TYPE.match(i))
                if held:
                    for d in ds:
                        plan.skipped.append((d, f"placeholder type {held[0]}: left for the type-naming issues"))
                    continue
                fwd, inc, problems = self.resolve_names(idents, defn.path, home)
                if problems:
                    for d in ds:
                        plan.skipped.append((d, "; ".join(problems)))
                    continue
                homed[(home, name)] = (kind, defn, text, fwd, inc)
        plan.homed = homed
        self.plan_drift(plan, left_out)
        for (home, name), (kind, defn, text, fwd, inc) in sorted(homed.items()):
            if kind == "func" and text is not None and self.default_arguments([], defn):
                edits = self.strip_default_edits(defn)
                have = {self.resolve_include(defn.path, k, n) for _, k, n in self.files[defn.path].includes}
                at = defn.item.marker_line[0] if defn.item.marker_line else defn.item.begin
                if home not in have:
                    edits.append((at, at, f'#include "{self.relative(defn.path, home)}"\n'))
                plan.default_edits.setdefault(defn.path, []).extend(edits)
        for d in decls:
            if d.defn is None or (d.kind, d.name) in bad or (d.path, d.home) in blocked:
                continue
            if (d.home, d.name) not in homed:
                continue
            plan.removals.setdefault(d.path, {}).setdefault(d.home, []).append(d)
        plan.blocked = sorted(blocked)
        return plan

    def plan_drift(self, plan, left_out):
        """Rewrite the declaration in a home header that no longer agrees with its definition."""
        for kind, defs in (("func", self.funcdefs), ("var", self.vardefs)):
            for name, cands in sorted(defs.items()):
                if len(cands) != 1 or name in left_out or name not in self.hdecls:
                    continue
                defn = cands[0]
                if defn.path == LINK_DATA:
                    continue
                home = self.home_of_cpp(defn.path)
                for h in self.hdecls[name]:
                    if h.path != home or (kind == "func") != (h.item.kind == "func_proto"):
                        continue
                    if kind == "func":
                        diffs = self.func_diffs(h.path, h.item, defn.path, defn.item)
                    else:
                        diffs = self.var_diffs(h.item, defn.item)
                    if not diffs:
                        continue
                    text, idents = self.render(kind, defn, self.default_arguments([h], defn) if kind == "func" else None)
                    held = sorted(i for i in idents if PLACEHOLDER_TYPE.match(i))
                    if held:
                        plan.skipped.append((Decl(h.path, h.item, kind, name, defn=defn), f"placeholder type {held[0]}: left for the type-naming issues"))
                        continue
                    fwd, inc, problems = self.resolve_names(idents, defn.path, home)
                    if problems:
                        plan.skipped.append((Decl(h.path, h.item, kind, name, defn=defn), "; ".join(problems)))
                        continue
                    plan.rewrites.setdefault(home, []).append(((h.item.begin, h.item.end, text), fwd, inc))
                    plan.header_mismatches.append((h, defn, diffs))

    def default_arguments(self, ds, defn):
        """Default arguments by parameter index: the definition's, else a prototype's."""
        found = {}
        sigs = [(defn.path, defn.item)] + [(d.path, d.item) for d in ds]
        for path, item in sigs:
            for k, text in enumerate(split_commas(item.params) if item.params.strip() else []):
                dflt = parse_param(text).default
                if dflt and k not in found:
                    found[k] = dflt
        return found

    def strip_default_edits(self, defn):
        """Edits that remove the default arguments from a definition."""
        edits = []
        text = self.files[defn.path].text
        for b, e in self.param_spans(defn.path, defn.item):
            k = top_level(self.files[defn.path].masked[b:e], "=")
            if k >= 0:
                start = b + k
                while start > b and text[start - 1] in " \t\n":
                    start -= 1
                edits.append((start, e, ""))
        return edits


    def guards(self):
        out = set()
        for path, src in self.files.items():
            if path.endswith(".h"):
                m = re.search(r"^#\s*ifndef\s+(\w+)", src.text, re.M)
                if m:
                    out.add(m.group(1))
        return out

    def new_guard(self, path, taken):
        p = Path(path)
        names = [p.stem] + [q.name for q in list(p.parents)[:-1]]
        for k in range(1, len(names) + 1):
            guard = re.sub(r"\W", "_", "_".join(reversed(names[:k])).upper()) + "_H"
            if guard not in taken:
                taken.add(guard)
                return guard
        return guard

    def relative(self, frm, to):
        import os
        return os.path.relpath(to, str(Path(frm).parent)).replace(os.sep, "/")

    def write_header(self, home, entries, taken, rewrites=()):
        """The new text of a home header given the declarations to add."""
        entries = sorted(entries, key=lambda e: (e[1].path, e[1].item.begin))
        fresh = home not in self.files
        incs = set().union(*[e[4] for e in entries], *[r[2] for r in rewrites]) - {home}
        fwds = set().union(*[e[3] for e in entries], *[r[1] for r in rewrites])
        decls = [e[2] for e in entries]
        if fresh:
            guard = self.new_guard(home, taken)
            lines = [f"#ifndef {guard}", f"#define {guard}", ""]
            if incs:
                lines += [include_line(home, i, self.relative) for i in sorted(incs, key=lambda i: (not i.startswith("<"), i))] + [""]
            if fwds:
                lines += sorted(fwds) + [""]
            lines += decls + ["", "#endif", ""]
            return "\n".join(lines)
        src = self.files[home]
        text = self.apply_edits(src.text, [r[0] for r in rewrites])
        visible = set(self.closure(home))
        incs = {i for i in incs if i not in visible and i != home}
        direct = {self.resolve_include(home, k, n) for _, k, n in src.includes}
        direct |= {f"<{n}>" for _, k, n in src.includes if k == "<"}
        incs -= direct
        first_item = min([it.begin for it in src.items if it.kind != "other" or it.text] or [len(text)])
        pos = 0
        m = re.search(r"^#\s*define\s+\w+[^\n]*\n", text, re.M)
        if re.match(r"\s*#\s*ifndef", text) and m:
            pos = m.end()
        elif re.match(r"\s*#\s*pragma\s+once", text):
            pos = text.index("\n") + 1 if "\n" in text else len(text)
        for off, kind, name in src.includes:
            if off < first_item:
                pos = max(pos, text.index("\n", off) + 1 if "\n" in text[off:] else len(text))
        add = []
        if incs:
            add += [include_line(home, i, self.relative) for i in sorted(incs, key=lambda i: (not i.startswith("<"), i))]
        if fwds:
            add += sorted(fwds)
        endif = [m.start() for m in re.finditer(r"^#\s*endif\b", text, re.M)]
        guarded = re.match(r"\s*#\s*ifndef", text) is not None and endif
        out = []
        tail_pos = endif[-1] if guarded else len(text)
        chunk = "\n".join(decls) + "\n"
        before = text[:tail_pos]
        sep = "" if before.endswith("\n\n") or not before.strip() else ("\n" if before.endswith("\n") else "\n\n")
        tail = chunk + ("\n" if guarded else "")
        new = before + sep + tail + text[tail_pos:] if decls else text
        if add:
            ins = "\n".join(add) + "\n"
            if not text[:pos].endswith("\n\n") and not text[pos:].startswith("\n"):
                ins += "\n"
            new_pos = pos
            new = new[:new_pos] + ins + new[new_pos:]
        if not new.endswith("\n"):
            new += "\n"
        return new

    def apply_edits(self, text, edits):
        for b, e, r in sorted(edits, key=lambda x: (x[0], x[1]), reverse=True):
            text = text[:b] + r + text[e:]
        return text

    def removal_span(self, text, begin, end, keep_blank=False):
        ls = text.rfind("\n", 0, begin) + 1
        full = not text[ls:begin].strip()
        le = end
        while le < len(text) and text[le] in " \t":
            le += 1
        if full and le < len(text) and text[le] == "\n":
            le += 1
            if not keep_blank and text[:ls].endswith("\n\n") and text[le:le + 1] == "\n":
                le += 1
            return ls, le, True
        if full and le >= len(text):
            return ls, le, True
        return begin, end, False

    def cpp_edits(self, path, groups):
        """The edits that remove a file's declarations and add the includes."""
        text = self.files[path].text
        edits = []
        includes = self.files[path].includes
        left = list(self.files[path].masked)
        for ds in groups.values():
            for d in ds:
                for k in range(d.item.begin, min(d.item.end, len(left))):
                    left[k] = " "
        left = "".join(left)
        for home, ds in sorted(groups.items()):
            inc = f'#include "{self.relative(path, home)}"' if home else ""
            real = [d for d in ds if home]
            first = min([d.item.begin for d in real] or [0])
            unused = not any(re.search(r"\b" + re.escape(d.name) + r"\b", left) for d in real)
            put = not real or home == path or unused or any(off < first and self.resolve_include(path, k, n) == home for off, k, n in includes)
            for d in sorted(ds, key=lambda d: d.item.begin):
                b, e, full = self.removal_span(text, d.item.begin, d.item.end, keep_blank=not put)
                if not put and home:
                    put = True
                    repl = inc + "\n" if full else "\n" + inc + "\n"
                    edits.append((b, e, repl))
                else:
                    edits.append((b, e, ""))
        return edits


    def outputs(self, plan):
        """{path: new text} of every file a write changes."""
        out = {}
        taken = self.guards()
        by_home = {}
        for (home, name), entry in sorted(plan.homed.items()):
            if entry[2] is not None:
                by_home.setdefault(home, []).append(entry)
        for home in plan.rewrites:
            by_home.setdefault(home, [])
        for home, entries in sorted(by_home.items()):
            new = self.write_header(home, entries, taken, plan.rewrites.get(home, ()))
            if home not in self.files or new != self.files[home].text:
                out[home] = new
        own = {}
        for path in self.cpps:
            home = self.home_of_cpp(path)
            if path != LINK_DATA and (home in self.files or home in out):
                own[path] = home
        for path in sorted(set(plan.removals) | set(plan.default_edits) | set(own)):
            edits = self.cpp_edits(path, plan.removals.get(path, {}))
            extra = plan.default_edits.get(path)
            if extra:
                edits += extra
            if path in own:
                edits += self.self_include_edit(path, own[path], edits, plan, out)
            if edits:
                out[path] = self.apply_edits(self.files[path].text, edits)
        return out

    def self_include_edit(self, path, home, edits, plan, out):
        """The edit that makes a .cpp include its own header, [] when it has, or when that header conflicts with it."""
        rel = self.relative(path, home)
        line = f'#include "{rel}"'
        if any(line in e[2] for e in edits) or any(self.resolve_include(path, k, n) == home for _, k, n in self.files[path].includes):
            return []
        plan.self_needed.add(path)
        if path in self.no_self_include:
            plan.no_self_include.append((path, self.no_self_include[path]))
            return []
        if home in self.files and self.type_conflict(path, home):
            plan.no_self_include.append((path, f"{self.type_conflict(path, home)} is defined again"))
            return []
        src = self.files[path]
        first = min([it.begin for it in src.items if it.kind != "other" or it.text] or [len(src.text)])
        first = min([first] + [it.marker_line[0] for it in src.items if it.begin == first and it.marker_line])
        last = [off for off, _, _ in src.includes if off < first]
        if last:
            at = src.text.index("\n", max(last)) + 1 if "\n" in src.text[max(last):] else len(src.text)
            return [(at, at, line + "\n")]
        at = src.text.rfind("\n", 0, first) + 1
        return [(at, at, line + "\n\n")]


@dataclass
class Plan:
    removals: dict = field(default_factory=dict)
    homed: dict = field(default_factory=dict)
    mismatches: list = field(default_factory=list)
    ambiguous: list = field(default_factory=list)
    nodef: list = field(default_factory=list)
    skipped: list = field(default_factory=list)
    kept_marker: list = field(default_factory=list)
    blocked: list = field(default_factory=list)
    type_conflicts: list = field(default_factory=list)
    nodef_defined: list = field(default_factory=list)
    bad_names: list = field(default_factory=list)
    left: list = field(default_factory=list)
    default_edits: dict = field(default_factory=dict)
    header_mismatches: list = field(default_factory=list)
    rewrites: dict = field(default_factory=dict)
    no_self_include: list = field(default_factory=list)
    self_needed: set = field(default_factory=set)


def decl_line(w, d):
    return f"{d.path}:{d.item.line}"


def one_line(text, limit=160):
    text = re.sub(r"\s+", " ", text).strip()
    return text if len(text) <= limit else text[:limit - 3] + "..."


def violation_rows(w, plan):
    """(location, message, kind, name) for each declaration a write leaves in a .cpp."""
    out = []
    for d in plan.mismatches:
        out.append((decl_line(w, d), f"{d.name} disagrees with its definition in {d.defn.path}: " + "; ".join(d.diffs), "mismatch", d.name))
    for d in plan.ambiguous:
        out.append((decl_line(w, d), f"{d.name} has several definitions: " + "; ".join(d.diffs), "ambiguous", d.name))
    for d, why in plan.skipped:
        out.append((decl_line(w, d), f"{d.name} cannot be declared in a header: {why}", "skip", d.name))
    for d in plan.nodef_defined:
        out.append((decl_line(w, d), f"{d.name} is not defined here", "undefined", d.name))
    return sorted(set(out))


def violations(w, plan):
    """(location, message) for each declaration a write leaves in a .cpp."""
    return [(loc, msg) for loc, msg, _, _ in violation_rows(w, plan)]


def load_left_out(root):
    """{(name, kind): reason} from the committed list of declarations the tool leaves in place."""
    path = Path(root) / LEFT_OUT
    if not path.exists():
        return {}
    import csv
    with path.open(newline="") as f:
        return {(r["name"], r["kind"]): r["reason"] for r in csv.DictReader(f)}


def load_no_self_include(root):
    """{path: reason} from the committed list of .cpp files that cannot include their own header."""
    path = Path(root) / NO_SELF_INCLUDE
    if not path.exists():
        return {}
    import csv
    with path.open(newline="") as f:
        return {r["path"]: r["reason"] for r in csv.DictReader(f)}


def check_left_out(w, plan, listed):
    """[(location, message)] for the declarations left in a .cpp that the list does not name, and the listed names with none."""
    rows = violation_rows(w, plan)
    found = [(loc, msg) for loc, msg, kind, name in rows if (name, kind) not in listed]
    present = {(name, kind) for _, _, kind, name in rows}
    found += [(LEFT_OUT, f"{name} ({kind}) is listed but needs no listing: remove the row") for name, kind in sorted(listed) if (name, kind) not in present]
    found += [(NO_SELF_INCLUDE, f"{p} is listed but includes its own header or has none: remove the row") for p in sorted(w.no_self_include) if p not in plan.self_needed]
    return found


def report(w, plan, outs):
    lines = []
    created = sorted(p for p in outs if p not in w.files and p.endswith(".h"))
    updated = sorted(p for p in outs if p in w.files and p.endswith(".h"))
    changed = sorted(p for p in outs if p.endswith(".cpp"))
    removed_f = sum(1 for g in plan.removals.values() for ds in g.values() for d in ds if d.kind == "func")
    removed_v = sum(1 for g in plan.removals.values() for ds in g.values() for d in ds if d.kind == "var")
    added_inc = sum(len(g) for g in plan.removals.values())
    lines.append(f"headers created: {len(created)}")
    lines.append(f"headers updated: {len(updated)}")
    lines.append(f"files changed: {len(changed)}")
    lines.append(f"prototypes removed: {removed_f}")
    lines.append(f"externs removed: {removed_v}")
    lines.append(f"includes added: {added_inc}")
    lines.append(f"files that do not include their own header: {len(plan.no_self_include)}")
    lines.append(f"mismatches: {len(plan.mismatches)}")
    lines.append(f"ambiguities: {len(plan.ambiguous)}")
    lines.append(f"skips (type not nameable in a header): {len(plan.skipped)}")
    lines.append(f"kept (GLOBAL marker on the extern): {len(plan.kept_marker)}")
    lines.append(f"files that keep their link/globals.h externs: {len(plan.blocked)}")
    lines.append(f"names left undeclared because a declaration of them disagrees or is ambiguous: {len(plan.bad_names)}")
    lines.append(f"declarations with no definition in the tree: {len(plan.nodef)}")
    lines.append(f"names declared in headers: {len(plan.homed)}")
    lines.append(f"home declarations rewritten to their definition: {len(plan.header_mismatches)}")
    lines.append("")
    lines.append("== Headers created")
    for p in created:
        lines.append(f"{p}: {sum(1 for (h, n) in plan.homed if h == p)} names")
    lines.append("")
    lines.append("== Headers updated")
    for p in updated:
        lines.append(f"{p}: {sum(1 for (h, n), e in plan.homed.items() if h == p and e[2] is not None)} names added")
    lines.append("")
    lines.append("== Files changed (prototypes, externs removed; includes added)")
    for p in changed:
        g = plan.removals.get(p, {})
        pf = sum(1 for ds in g.values() for d in ds if d.kind == "func")
        pv = sum(1 for ds in g.values() for d in ds if d.kind == "var")
        lines.append(f"{p}: -{pf} prototypes -{pv} externs +{len(g)} includes")
    lines.append("")
    lines.append("== Mismatches (declaration | definition | differences); left in place, the definition wins once fixed")
    for d in sorted(plan.mismatches, key=lambda d: (d.path, d.item.line)):
        lines.append(f"{decl_line(w, d)}: {d.name}")
        lines.append(f"    declared: {one_line(d.item.text)}")
        lines.append(f"    defined:  {d.defn.path}:{d.defn.item.line} {one_line(d.defn.item.text.split('{')[0])}")
        lines.append(f"    differs:  {'; '.join(d.diffs)}")
    lines.append("")
    lines.append("== Home declarations that drifted from their definition (rewritten)")
    for h, defn, diffs in sorted(plan.header_mismatches, key=lambda x: (x[0].path, x[0].item.line)):
        lines.append(f"{h.path}:{h.item.line}: {h.item.name}: {'; '.join(diffs)}")
    lines.append("")
    lines.append("== Files that do not include their own header (rule: it would define a type a second time; or a row of data/headers_no_self_include.csv)")
    for p, why in sorted(plan.no_self_include):
        lines.append(f"{p}: {why}")
    lines.append("")
    lines.append("== Ambiguities (several definitions; left in place)")
    for d in sorted(plan.ambiguous, key=lambda d: (d.path, d.item.line)):
        lines.append(f"{decl_line(w, d)}: {d.name}: {'; '.join(d.diffs)}")
    lines.append("")
    lines.append("== Skips (left in place)")
    for d, why in sorted(plan.skipped, key=lambda x: (x[0].path, x[0].item.line)):
        lines.append(f"{decl_line(w, d)}: {d.name}: {why}")
    lines.append("")
    lines.append("== Kept: externs with a // GLOBAL: marker")
    for d in sorted(plan.kept_marker, key=lambda d: (d.path, d.item.line)):
        lines.append(f"{decl_line(w, d)}: {d.name}")
    lines.append("")
    lines.append("== Files that keep their link/globals.h externs because one disagrees")
    for f, h in plan.blocked:
        lines.append(f"{f}")
    lines.append("")
    lines.append("== Names left undeclared (all their declarations stay)")
    for kind, name in plan.bad_names:
        lines.append(f"{kind} {name}")
    lines.append("")
    lines.append("== Declarations with no definition in the tree (imports, or stale)")
    for name in sorted({d.name for d in plan.nodef}):
        lines.append(name)
    return "\n".join(lines) + "\n"


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--check", action="store_true", help="exit 1 if a .cpp declares what it does not define and data/headers_left_out.csv does not list it, if a listed name needs no listing, or a write would change a file")
    ap.add_argument("--dry-run", action="store_true", help="print the report, change nothing")
    ap.add_argument("--report", metavar="FILE", help="write the report to FILE, change nothing")
    ap.add_argument("--root", default=str(ROOT), help=argparse.SUPPRESS)
    args = ap.parse_args(argv)
    w = World(args.root)
    left_out = load_left_out(args.root)
    plan = w.plan(left_out={name for name, _ in left_out})
    outs = w.outputs(plan)
    if args.report or args.dry_run:
        text = report(w, plan, outs)
        if args.report:
            Path(args.report).write_text(text)
        else:
            sys.stdout.write(text)
        return 0
    if args.check:
        found = check_left_out(w, plan, left_out)
        for loc, msg in found:
            print(f"{loc}: {msg}")
        for p in sorted(outs):
            print(f"would change {p}")
        return 1 if found or outs else 0
    for p, text in sorted(outs.items()):
        file = w.root / p
        file.parent.mkdir(parents=True, exist_ok=True)
        file.write_bytes(text.encode("utf-8", errors="surrogateescape"))
    print(f"changed {len(outs)} files")
    for loc, msg in violations(w, plan):
        print(f"{loc}: {msg}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
