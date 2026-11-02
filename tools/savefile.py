"""Compare two HAPIBANK save files (the game's savegame\\harness.sav) by their decoded contents.

A save's account chunks are LZSS-compressed (SQSH), so one uninitialised byte shifts every later
byte of the file. `differences` therefore decodes both files and compares them account by account,
item by item and box by box, leaving out the bytes the format takes from the clock or from
uninitialised stack and padding (MASKS, each with its offset and reason).

    uv run tools/savefile.py STORED GOT       # prints every difference; exit 1 when there is one
"""

import re
import struct
import sys
from dataclasses import dataclass
from pathlib import Path

HEADER_SIZE = 0x22
ACCOUNT_HEADER_SIZE = 0x20
CHUNK_HEADER_SIZE = 0x13
NAME_FIELD = 0x20
SAVE_RECORD_SIZE = 0xB8
SAVE_PIECE_FIRST = 65
SAVE_PIECE_SIZE = 24
SAVE_FLAGS_BYTE = 182
MOTION_RECORD_SIZE = 0x23
POINT_MARKER_SIZE = 20


class SaveError(Exception):
    pass


def lzss_expand(src: bytes) -> bytes:
    """The output of the game's LzssExpand (0x4d1480) over `src`."""
    window = bytearray(0x1011)
    pos = 1
    out = bytearray()
    i = 0
    flags = src[i]
    i += 1
    mask = 1
    while True:
        if not flags & mask:
            c = src[i]
            i += 1
            out.append(c)
            window[pos] = c
            pos = (pos + 1) & 0xFFF
        else:
            word = src[i] | src[i + 1] << 8
            i += 2
            dist = word >> 4
            if dist == 0:
                return bytes(out)
            for k in range((word & 15) + 2):
                c = window[(dist + k) & 0xFFF]
                out.append(c)
                window[pos] = c
                pos = (pos + 1) & 0xFFF
        mask <<= 1
        if mask & 0x100:
            mask = 1
            flags = src[i]
            i += 1


def unsquash(chunk: bytes) -> bytes:
    """The bytes a SQSH chunk (method 1, LZSS) holds."""
    if chunk[:4] != b"SQSH":
        raise SaveError("a compressed chunk has no SQSH header")
    method, encrypt = chunk[5], chunk[6]
    packed, size, checksum = struct.unpack_from("<III", chunk, 7)
    data = bytearray(chunk[CHUNK_HEADER_SIZE:CHUNK_HEADER_SIZE + packed])
    if sum(data) & 0xFFFFFFFF != checksum:
        raise SaveError("a SQSH chunk's checksum does not match")
    if method != 1:
        raise SaveError(f"a SQSH chunk has method {method}, not 1")
    if encrypt:
        for i in range(len(data)):
            data[i] = ((data[i] - i) ^ i) & 255
    out = lzss_expand(bytes(data))
    if len(out) != size:
        raise SaveError("a SQSH chunk expanded to the wrong size")
    return out


def cstring(pool: bytes, offset: int) -> str:
    return pool[offset:pool.index(0, offset)].decode("latin1")


@dataclass
class Box:
    key: str
    data: bytes
    named: bool
    table: tuple[int, int, int, int]


@dataclass
class Account:
    name: str
    header: tuple[int, ...]
    items: dict[str, tuple]
    boxes: list[Box]


@dataclass
class Bank:
    header: bytes
    pool: bytes
    accounts: list[Account]


def parse(data: bytes) -> Bank:
    """The decoded bank: header, string pool and accounts; offsets that depend on compression are normalised."""
    if data[:8] != b"HAPIBANK":
        raise SaveError("not a HAPIBANK file")
    _, name_offset, pool_offset, header_size, version = struct.unpack_from("<8siiii", data)
    pool = data[pool_offset:]
    if data[24]:
        pool = unsquash(pool)
    accounts = []
    start = header_size
    while start < pool_offset:
        size, str_offset, n_ints, n_doubles, n_strings, n_boxes, compressed, unknown = struct.unpack_from("<8i", data, start)
        body = data[start + ACCOUNT_HEADER_SIZE:start + size]
        if compressed == 1:
            body = unsquash(body)
        items: dict[str, tuple] = {}
        p = 0
        for _ in range(n_ints):
            name, value = struct.unpack_from("<ii", body, p)
            p += 8
            items["int " + cstring(pool, name)] = (value,)
        for _ in range(n_doubles):
            name, value = struct.unpack_from("<i8s", body, p)
            p += 12
            items["double " + cstring(pool, name)] = (value,)
        for _ in range(n_strings):
            name, value = struct.unpack_from("<ii", body, p)
            p += 8
            items["string " + cstring(pool, name)] = (cstring(pool, value),)
        boxes = []
        table = p
        for index in range(n_boxes):
            name, number, offset, length = struct.unpack_from("<iiii", body, table + index * 16)
            relative = offset - start - ACCOUNT_HEADER_SIZE
            named = name != -1
            key = cstring(pool, name) if named else f"#{number}"
            boxes.append(Box(key, body[relative:relative + length], named, (name, -1 if named else number, relative, length)))
        accounts.append(Account(cstring(pool, str_offset), (n_ints, n_doubles, n_strings, n_boxes, version, unknown), items, boxes))
        start += size
    return Bank(data[:12] + data[16:HEADER_SIZE], pool, accounts)


@dataclass(frozen=True)
class Mask:
    """Bytes of the boxes named `box` (a regular expression) of `account`, `size` bytes long, that are not compared.

    `offset` and `bits` (the bits of that byte left out) say where. With `after_nul`, `offset` is where a
    NUL terminated string starts and the bytes after its terminator, up to `length`, are left out.
    With `whole`, every byte of a box of any size is left out.
    """

    account: str
    box: str
    size: int
    offset: int
    bits: int
    why: str
    after_nul: bool = False
    length: int = 1
    whole: bool = False


NOT_COMPARED = (
    ("header offset 12 (poolOffset)", "follows the compressed sizes of the accounts before the pool"),
    ("account header size (offset 0 of each account)", "the compressed size of the account"),
    ("box table, word 2 (offset 4 of an entry) of a named box", "never set by SaveBank: uninitialised stack"),
    ("box table, word 3 (offset 8 of an entry)", "an absolute file offset, compared relative to its account"),
)

CLOCK = "read from the host clock (GetTicks)"
STACK = "never set by the save code: uninitialised stack"
PADDING = "never set by the save code: uninitialised bit-field padding"
UNIT_BOX = r"#\d+"
MOTION_BOX = r"u[0-9a-f]{4}mob"
MARKER_BOX = r"u[0-9a-f]{4}m[0-9a-f]{4}g"

MASKS: tuple[Mask, ...] = (
    Mask("Players", "GameTime", 28, 0, 0xFF, "Game::lastTick: " + CLOCK, length=4),
    Mask("Units", UNIT_BOX, SAVE_RECORD_SIZE, 0, 0xFF, "SaveRec::name after its terminator: " + STACK, after_nul=True, length=NAME_FIELD),
    *(Mask("Units", UNIT_BOX, SAVE_RECORD_SIZE, SAVE_PIECE_FIRST + k * SAVE_PIECE_SIZE + 23, 0xE0, "SrcPiece PieceBits::b5: " + PADDING)
      for k in range(3)),
    Mask("Units", UNIT_BOX, SAVE_RECORD_SIZE, SAVE_FLAGS_BYTE, 0x0E, "SaveRec::flags FlagBits::d: " + PADDING),
    Mask("Units", MOTION_BOX, MOTION_RECORD_SIZE, MOTION_RECORD_SIZE - 1, 0xF8, "UnitMotionSaveRecord bits above mode and flag: " + PADDING),
    Mask("Units", MARKER_BOX, POINT_MARKER_SIZE, 0, 0xFF, "PointMarkerRecord::reserved: " + STACK, length=4),
    Mask("Summary", "Radar Image", 0, 0, 0xFF, "the radar draw moved into the drain, so the stored preview's pixels differ", whole=True),
)


def kept(account: str, box: Box) -> bytearray:
    """For each byte of the box, the bits that are compared."""
    keep = bytearray(b"\xff" * len(box.data))
    for mask in MASKS:
        if mask.account != account or not re.fullmatch(mask.box, box.key):
            continue
        if mask.whole:
            return bytearray(len(box.data))
        if mask.size != len(box.data):
            continue
        if mask.after_nul:
            end = box.data.index(0, mask.offset) + 1 if 0 in box.data[mask.offset:mask.length] else mask.length
            for i in range(end, mask.length):
                keep[i] = 0
        else:
            for i in range(mask.offset, mask.offset + mask.length):
                keep[i] &= ~mask.bits & 0xFF
    return keep


def box_difference(account: str, stored: Box, got: Box) -> str | None:
    if len(stored.data) != len(got.data):
        return f"{account}/{stored.key}: stored {len(stored.data)} bytes, got {len(got.data)}"
    keep_stored, keep_got = kept(account, stored), kept(account, got)
    for i, (a, b) in enumerate(zip(stored.data, got.data)):
        if (a ^ b) & keep_stored[i] & keep_got[i]:
            return f"{account}/{stored.key} offset {i:#x}: stored {a:#04x}, got {b:#04x}"
    return None


def differences(stored: bytes, got: bytes) -> list[str]:
    """Every difference between two saves, each naming the account, the item or box and the offset in it."""
    try:
        a, b = parse(stored), parse(got)
    except (SaveError, struct.error, ValueError, IndexError) as e:
        return [f"save does not decode: {e}"]
    out: list[str] = []
    if a.header != b.header:
        out.append("save header differs")
    if a.pool != b.pool:
        out.append("save string pool differs")
    if [x.name for x in a.accounts] != [x.name for x in b.accounts]:
        return out + ["save accounts differ: " + ", ".join(x.name for x in a.accounts) + " vs " + ", ".join(x.name for x in b.accounts)]
    for x, y in zip(a.accounts, b.accounts):
        if x.header != y.header:
            out.append(f"{x.name}: item and box counts differ")
            continue
        for key in x.items:
            if x.items[key] != y.items.get(key):
                out.append(f"{x.name}: {key}: stored {x.items[key]}, got {y.items.get(key)}")
        for p, q in zip(x.boxes, y.boxes):
            if (p.key, p.table[3]) != (q.key, q.table[3]) or p.table[2] != q.table[2]:
                out.append(f"{x.name}: box {p.key} differs in name, length or place")
                continue
            diff = box_difference(x.name, p, q)
            if diff:
                out.append(diff)
    return out


def main() -> None:
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    found = differences(Path(sys.argv[1]).read_bytes(), Path(sys.argv[2]).read_bytes())
    print("\n".join(found) or "equal")
    sys.exit(1 if found else 0)


if __name__ == "__main__":
    main()
