#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include "../../interfaces/ref.h"
#include "replay_schema.h"
#pragma pack(push, 1)
#include "../game/game.h"
#pragma pack(pop)
#include "../game/los_helpers.h"
#include "../network/player.h"
#include "../state/sim_globals.h"
#include "../units/unit.h"
#include "replay_text.h"
#include "replay_types.h"
#include "replay_objects.h"

namespace {

unsigned PointerHash(const void* p)
{
    unsigned v = (unsigned)(size_t)p;
    return (v ^ (v >> 7) ^ (v >> 15)) % 16384u;
}

unsigned ParseHex8(const char* p)
{
    unsigned v = 0;
    for (int i = 0; i < 8; i++) {
        char c = p[i];
        v = v << 4 | (unsigned)(c <= '9' ? c - '0' : c - 'a' + 10);
    }
    return v;
}

unsigned char* BlockAt(unsigned offset)
{
    return offset == 0 ? nullptr : (unsigned char*)SimBlockBase() + offset;
}

unsigned FactorValue(const ReplayFactor& f, const void* object)
{
    if (f.size == 0)
        return f.offset;
    const unsigned char* at = (const unsigned char*)object + f.offset;
    unsigned v = f.size == 1 ? *at : f.size == 2 ? *(const unsigned short*)at : *(const unsigned*)at;
    return v >> f.shift;
}

unsigned HexDigit(char c)
{
    return (unsigned)(c <= '9' ? c - '0' : (c | 0x20) - 'a' + 10);
}

bool TableSpan(unsigned kind, unsigned char** base, unsigned* count, unsigned* stride)
{
    unsigned table = kReplayObjectKinds[kind].table;
    if (table == RT_ORDER)
        return false;
    const ReplayTableSpec& spec = kReplayTables[table - 1];
    unsigned char* root = spec.sim ? (unsigned char*)g_simGlobals : (unsigned char*)g_game;
    if (root == nullptr)
        return false;
    *base = spec.ref ? BlockAt(*(const unsigned*)(root + spec.offset)) : root + spec.offset;
    *count = ReplayLengthOf(spec.count, root);
    *stride = kReplayObjectKinds[kind].size;
    return *base != nullptr;
}

void StoreFactor(const ReplayFactor& f, void* object, unsigned value)
{
    unsigned char* at = (unsigned char*)object + f.offset;
    if (f.size == 1)
        *at = (unsigned char)value;
    else if (f.size == 2)
        *(unsigned short*)at = (unsigned short)value;
    else if (f.size == 4)
        *(unsigned*)at = value;
}

void EnsureTable(unsigned kind, ReplayAllocFn alloc)
{
    const ReplayTableSpec& spec = kReplayTables[kReplayObjectKinds[kind].table - 1];
    unsigned char* root = spec.sim ? (unsigned char*)g_simGlobals : (unsigned char*)g_game;
    if (root == nullptr || !spec.ref || spec.alloc == 0 || alloc == nullptr || *(const unsigned*)(root + spec.offset) != 0)
        return;
    unsigned size = spec.alloc * kReplayObjectKinds[kind].size;
    unsigned char* pool = (unsigned char*)alloc(size);
    memset(pool, 0, size);
    *(unsigned*)(root + spec.offset) = (unsigned)(pool - (unsigned char*)SimBlockBase());
    StoreFactor(spec.count.first, root, spec.alloc);
}

int TableIndex(unsigned kind, const void* p)
{
    unsigned char* base;
    unsigned count;
    unsigned stride;
    if (p == nullptr || !TableSpan(kind, &base, &count, &stride))
        return -1;
    const unsigned char* q = (const unsigned char*)p;
    if (q < base || (unsigned)(q - base) % stride != 0 || (unsigned)(q - base) / stride >= count)
        return -1;
    return (int)((unsigned)(q - base) / stride);
}

bool InsideTable(unsigned kind, const void* p, unsigned* container, int* index)
{
    const ReplayObjectKind& k = kReplayObjectKinds[kind];
    if (!k.hasInside || p == nullptr)
        return false;
    int at = TableIndex(k.insideKind, (const unsigned char*)p - k.insideOffset);
    if (at < 0)
        return false;
    *container = k.insideKind;
    *index = at;
    return true;
}

}

namespace {

char s_failure[240];

}

void ReplayFail(const char* message)
{
    if (s_failure[0] == '\0') {
        strncpy(s_failure, message, sizeof s_failure - 1);
        s_failure[sizeof s_failure - 1] = '\0';
    }
}

const char* ReplayFailure()
{
    return s_failure[0] == '\0' ? nullptr : s_failure;
}

void ReplayClearFailure()
{
    s_failure[0] = '\0';
}

bool ReplayKindFits(unsigned declared, unsigned actual)
{
    if (declared == actual)
        return true;
    const ReplayObjectKind& base = kReplayObjectKinds[declared];
    for (unsigned i = 0; i < base.polyCount; i++) {
        if (kReplayPolyKinds[base.polyFirst + i] == actual)
            return true;
    }
    return false;
}

unsigned ReplayConcreteKind(unsigned kind, const void* p)
{
    const ReplayObjectKind& base = kReplayObjectKinds[kind];
    if (base.polyCount == 0 || p == nullptr)
        return kind;
    const void* vptr = *(const void* const*)p;
    for (unsigned i = 0; i < base.polyCount; i++) {
        unsigned concrete = kReplayPolyKinds[base.polyFirst + i];
        if (ReplayVtable(concrete) == vptr)
            return concrete;
    }
    const char* symbol = nullptr;
    for (unsigned k = 0; k < kReplayObjectKindCount; k++) {
        if (ReplayVtable(k) != nullptr && ReplayVtable(k) == vptr)
            symbol = ReplayVtableName(k);
    }
    char message[200];
    if (symbol != nullptr)
        sprintf(message, "an %s has the vtable %s, which is none of its kinds", base.name, symbol);
    else
        sprintf(message, "an %s has a vtable that is none of its kinds and no vtable of the schema", base.name);
    ReplayFail(message);
    return kReplayObjectKindCount;
}

bool ReplayInsideTable(unsigned kind, const void* p)
{
    unsigned container;
    int index;
    return InsideTable(kind, p, &container, &index);
}

void ReplayObjectTable::Reset()
{
    for (unsigned k = 0; k < KINDS; k++) {
        count[k] = 0;
        memset(slots[k], 0xff, sizeof slots[k]);
    }
}

unsigned ReplayObjectTable::Find(unsigned kind, const void* p) const
{
    if (p == nullptr)
        return 0;
    if (kind < kReplayObjectKindCount && kReplayObjectKinds[kind].table != RT_ORDER) {
        int at = TableIndex(kind, p);
        return at < 0 ? 0 : (unsigned)at + 1;
    }
    for (unsigned i = PointerHash(p);; i = (i + 1) % HASH) {
        int at = slots[kind][i];
        if (at < 0)
            return 0;
        if (objects[kind][at] == p)
            return (unsigned)at + 1;
    }
}

unsigned ReplayObjectTable::Name(unsigned kind, const void* p)
{
    unsigned found = Find(kind, p);
    if (found != 0 || p == nullptr)
        return found;
    if (kReplayObjectKinds[kind].table != RT_ORDER)
        return 0;
    if (kind >= KINDS || count[kind] == REPLAY_MAX_OBJECTS)
        abort();
    Bind(kind, count[kind] + 1, (void*)p);
    return count[kind];
}

void* ReplayObjectTable::Pointer(unsigned kind, unsigned ordinal) const
{
    if (kReplayObjectKinds[kind].table != RT_ORDER) {
        unsigned char* base;
        unsigned count;
        unsigned stride;
        if (ordinal == 0 || !TableSpan(kind, &base, &count, &stride) || ordinal > count)
            return nullptr;
        return base + (ordinal - 1) * stride;
    }
    return ordinal >= 1 && ordinal <= count[kind] ? objects[kind][ordinal - 1] : nullptr;
}

void ReplayObjectTable::Bind(unsigned kind, unsigned ordinal, void* p)
{
    if (kind >= KINDS || ordinal == 0 || ordinal > REPLAY_MAX_OBJECTS)
        abort();
    while (count[kind] < ordinal)
        objects[kind][count[kind]++] = nullptr;
    objects[kind][ordinal - 1] = p;
    unsigned i = PointerHash(p);
    while (slots[kind][i] >= 0)
        i = (i + 1) % HASH;
    slots[kind][i] = (short)(ordinal - 1);
}

void ReplayAddWeapon(ReplayText& t, const void* p)
{
    if (p == nullptr) {
        t.Add("null");
        return;
    }
    const unsigned char* base = g_game == nullptr ? nullptr : (const unsigned char*)g_game->weapons;
    const unsigned char* q = (const unsigned char*)p;
    unsigned span = sizeof(WeaponDef);
    if (base != nullptr && q >= base && q < base + 256 * span && (unsigned)(q - base) % span == 0) {
        t.Add("weapondef#");
        t.AddUnsigned((unsigned)(q - base) / span);
    } else {
        t.Add("weapondef#?");
    }
}

void* ReplayParseWeapon(const char* value)
{
    if (strncmp(value, "weapondef#", 10) != 0)
        return nullptr;
    unsigned index = (unsigned)atoi(value + 10);
    return index < 256 && g_game != nullptr ? &g_game->weapons[index] : nullptr;
}

namespace {

enum { ARRAY_LINE_BYTES = 512, ARRAY_LINE_TEXT = 1200, MAX_ARRAY_ELEMENTS = 1 << 24 };

void AddHex(ReplayText& t, const unsigned char* p, unsigned bytes)
{
    for (unsigned i = 0; i < bytes; i++) {
        t.AddChar("0123456789abcdef"[p[i] >> 4]);
        t.AddChar("0123456789abcdef"[p[i] & 15]);
    }
}

bool ArrayData(const ReplayObjectField& f, const void* object, unsigned char** data, unsigned* count)
{
    const ReplayArraySpec& a = kReplayArrays[f.array - 1];
    unsigned char* at = (unsigned char*)object + f.offset;
    unsigned n = ReplayLengthOf(a.length, object);
    if (a.ref) {
        *data = BlockAt(*(const unsigned*)at);
        if (*data == nullptr)
            return false;
    } else {
        *data = at;
        if (n > a.capacity / a.elem)
            n = a.capacity / a.elem;
    }
    *count = n;
    return true;
}

void FormatArray(const ReplayObjectField& f, const void* object, ReplayText& out)
{
    unsigned char* data;
    unsigned count;
    if (!ArrayData(f, object, &data, &count)) {
        out.Add("null");
        return;
    }
    out.AddChar('[');
    out.AddUnsigned(count);
    out.AddChar(']');
    AddHex(out, data, count * kReplayArrays[f.array - 1].elem);
}

const char* ArrayHex(const char* text)
{
    return strchr(text, ']') + 1;
}

unsigned ArrayCount(const char* text)
{
    return text[0] == '[' ? (unsigned)strtoul(text + 1, nullptr, 10) : 0;
}

void ArrayLine(ReplayText& out, char tag, const char* key, const char* head)
{
    out.Add("  ");
    out.AddChar(tag);
    out.AddChar(' ');
    out.Add(key);
    out.AddChar(' ');
    out.Add(head);
}

void EmitArray(ReplayText& out, ReplayKnown& known, const char* key, char tag, const char* now, unsigned elem)
{
    const char* old = known.Get(key);
    if (old != nullptr && strcmp(old, now) == 0)
        return;
    if (strcmp(now, "null") == 0) {
        ArrayLine(out, tag, key, "null\n");
        known.Set(key, now);
        return;
    }
    unsigned count = ArrayCount(now);
    const char* hex = ArrayHex(now);
    unsigned perLine = (ARRAY_LINE_BYTES + elem - 1) / elem;
    char head[40];
    unsigned headLength = (unsigned)(strchr(now, ']') - now) + 1;
    memcpy(head, now, headLength);
    head[headLength] = '\0';
    bool delta = old != nullptr && strncmp(old, head, headLength) == 0;
    if (!delta) {
        if (count == 0) {
            ArrayLine(out, tag, key, head);
            out.AddChar('\n');
        }
        for (unsigned start = 0; start < count; start += perLine) {
            unsigned n = count - start < perLine ? count - start : perLine;
            ArrayLine(out, tag, key, start == 0 ? head : "");
            if (start == 0)
                out.AddChar(' ');
            out.AddUnsigned(start);
            out.AddChar(':');
            for (unsigned i = 0; i < n * elem * 2; i++)
                out.AddChar(hex[start * elem * 2 + i]);
            out.AddChar('\n');
        }
        known.Set(key, now);
        return;
    }
    const char* before = ArrayHex(old);
    unsigned width = elem * 2;
    unsigned lineStart = 0;
    bool open = false;
    for (unsigned i = 0; i < count;) {
        if (memcmp(hex + i * width, before + i * width, width) == 0) {
            i++;
            continue;
        }
        unsigned end = i + 1;
        while (end < count && end - i < perLine && memcmp(hex + end * width, before + end * width, width) != 0)
            end++;
        if (open && out.Size() - lineStart + 12 + (end - i) * width > ARRAY_LINE_TEXT) {
            out.AddChar('\n');
            open = false;
        }
        if (!open) {
            lineStart = out.Size();
            ArrayLine(out, tag, key, "");
            open = true;
        } else {
            out.AddChar(' ');
        }
        out.AddUnsigned(i);
        out.AddChar(':');
        for (unsigned k = 0; k < (end - i) * width; k++)
            out.AddChar(hex[i * width + k]);
        i = end;
    }
    if (open)
        out.AddChar('\n');
    known.Set(key, now);
}

}

namespace {

struct VectorView {
    unsigned char* data;
    unsigned length;
    unsigned capacity;
    unsigned size;
};

VectorView ViewOf(const ReplayObjectField& f, const void* object)
{
    VectorView view = {};
    view.size = kReplayVectors[f.array - 1].size;
    const unsigned* words = (const unsigned*)((const unsigned char*)object + f.offset + 4);
    if (words[0] == 0 || words[1] < words[0] || words[2] < words[1])
        return view;
    view.data = BlockAt(words[0]);
    view.length = (words[1] - words[0]) / view.size;
    view.capacity = (words[2] - words[0]) / view.size;
    return view;
}

void AddBracket(ReplayText& out, unsigned length, unsigned capacity)
{
    out.AddChar('[');
    out.AddUnsigned(length);
    out.AddChar('/');
    out.AddUnsigned(capacity);
    out.AddChar(']');
}

void FormatVector(const ReplayObjectField& f, const void* object, ReplayObjectTable& table, bool assign, ReplayText& out)
{
    VectorView view = ViewOf(f, object);
    const ReplayVectorSpec& spec = kReplayVectors[f.array - 1];
    AddBracket(out, view.length, view.capacity);
    if (spec.mode == RV_OBJECT) {
        for (unsigned i = 0; i < view.length; i++) {
            out.AddChar(i == 0 ? ' ' : ',');
            ReplayAddObjectName(out, table, spec.kind, BlockAt(((const unsigned*)view.data)[i]), assign);
        }
    } else if (spec.mode == RV_SCALAR) {
        AddHex(out, view.data, view.length * view.size);
    }
}

void FormatCode(const ReplayObjectField& f, const void* function, ReplayText& out)
{
    if (function == nullptr) {
        out.Add("null");
        return;
    }
    const ReplayCodeTable& table = kReplayCodeTables[f.target];
    unsigned i = 0;
    while (i < table.count && table.entries[i] != function)
        i++;
    out.Add(table.name);
    if (i == table.count) {
        out.Add("[?]");
        char message[200];
        sprintf(message, "%s is a code pointer that is none of the entries of the code table %s", f.name, table.name);
        ReplayFail(message);
        return;
    }
    out.AddChar('[');
    out.AddUnsigned(i);
    out.AddChar(']');
}

bool RestoreCode(const ReplayObjectField& f, const char* value, const void** into)
{
    if (strcmp(value, "null") == 0) {
        *into = nullptr;
        return true;
    }
    const ReplayCodeTable& table = kReplayCodeTables[f.target];
    size_t n = strlen(table.name);
    if (strncmp(value, table.name, n) != 0 || value[n] != '[')
        return false;
    char* end;
    unsigned long index = strtoul(value + n + 1, &end, 10);
    if (end == value + n + 1 || *end != ']' || end[1] != '\0' || index >= table.count)
        return false;
    *into = table.entries[index];
    return true;
}

}

unsigned ReplayLengthOf(const ReplayLength& length, const void* object)
{
    unsigned long long n = (unsigned long long)FactorValue(length.first, object) * FactorValue(length.second, object);
    return n > MAX_ARRAY_ELEMENTS ? MAX_ARRAY_ELEMENTS : (unsigned)n;
}

void ReplayAddObjectName(ReplayText& t, ReplayObjectTable& table, unsigned kind, const void* p, bool assign)
{
    if (p == nullptr) {
        t.Add("null");
        return;
    }
    if (kReplayObjectKinds[kind].scope == RS_INDEX) {
        ReplayAddWeapon(t, p);
        return;
    }
    unsigned actual = ReplayConcreteKind(kind, p);
    if (actual == kReplayObjectKindCount) {
        t.Add(kReplayObjectKinds[kind].name);
        t.Add("#?");
        return;
    }
    kind = actual;
    unsigned container;
    int index;
    if (InsideTable(kind, p, &container, &index)) {
        t.Add(kReplayObjectKinds[container].name);
        t.AddChar('#');
        t.AddUnsigned((unsigned)index);
        t.AddChar('.');
        t.Add(kReplayObjectKinds[kind].insideName);
        return;
    }
    unsigned ordinal = assign ? table.Name(kind, p) : table.Find(kind, p);
    t.Add(kReplayObjectKinds[kind].name);
    if (ordinal == 0) {
        t.Add("#?");
        return;
    }
    t.AddChar('#');
    t.AddUnsigned(kReplayObjectKinds[kind].table == RT_ORDER ? ordinal : ordinal - 1);
}

bool ReplayFormatObjectField(const ReplayObjectField& f, const void* object, ReplayObjectTable& table, bool assign, ReplayText& out)
{
    out.Clear();
    const unsigned char* at = (const unsigned char*)object + f.offset;
    switch (f.kind) {
    case RK_I32:
        out.AddInt(*(const int*)at);
        break;
    case RK_I16:
        out.AddInt(*(const short*)at);
        break;
    case RK_U32:
        out.AddUnsigned(*(const unsigned*)at);
        break;
    case RK_U16:
        out.AddUnsigned(*(const unsigned short*)at);
        break;
    case RK_F32:
        out.AddF32(*(const unsigned*)at);
        break;
    case RK_U8:
        out.AddUnsigned(*at);
        break;
    case RK_F64:
        out.AddF64(((const unsigned*)at)[0], ((const unsigned*)at)[1]);
        break;
    case RK_BYTES: {
        char hex[4];
        out.Add("x:");
        for (unsigned i = 0; i < f.size; i++) {
            sprintf(hex, "%02x", at[i]);
            out.Add(hex);
        }
        break;
    }
    case RK_FLAGS:
        out.AddHex32(*(const unsigned*)at);
        break;
    case RK_POINT16:
        out.AddChar('(');
        out.AddInt(((const short*)at)[0]);
        out.AddChar(',');
        out.AddInt(((const short*)at)[1]);
        out.AddChar(')');
        break;
    case RK_STRING: {
        char text[64];
        unsigned size = f.size < sizeof text - 1 ? f.size : (unsigned)sizeof text - 1;
        memcpy(text, at, size);
        text[size] = '\0';
        out.AddString(text);
        break;
    }
    case RK_WEAPON:
        ReplayAddWeapon(out, BlockAt(*(const unsigned*)at));
        break;
    case RK_REF:
        ReplayAddObjectName(out, table, f.target, BlockAt(*(const unsigned*)at), assign);
        break;
    case RK_UNITID:
        out.AddUnsigned(*(const unsigned short*)at);
        break;
    case RK_ARRAY:
        FormatArray(f, object, out);
        break;
    case RK_CODE:
        FormatCode(f, *(const void* const*)at, out);
        break;
    case RK_VECTOR:
        FormatVector(f, object, table, assign, out);
        break;
    default:
        return false;
    }
    return true;
}

int ReplayFindObjectKind(const char* name, unsigned length)
{
    for (unsigned k = 0; k < kReplayObjectKindCount; k++) {
        if (strlen(kReplayObjectKinds[k].name) == length && strncmp(kReplayObjectKinds[k].name, name, length) == 0)
            return (int)k;
    }
    return -1;
}

const ReplayObjectField* ReplayFindObjectField(unsigned kind, const char* name)
{
    for (unsigned i = 0; i < kReplayObjectFieldCount; i++) {
        if (kReplayObjectFields[i].object == kind && strcmp(kReplayObjectFields[i].name, name) == 0)
            return &kReplayObjectFields[i];
    }
    return nullptr;
}

namespace {

int ObjectTextKind(const char* text, unsigned length)
{
    const char* hash = (const char*)memchr(text, '#', length);
    if (hash == nullptr)
        return -1;
    int kind = ReplayFindObjectKind(text, (unsigned)(hash - text));
    if (kind < 0)
        return -1;
    const char* end = hash + 1;
    while (end < text + length && *end >= '0' && *end <= '9')
        end++;
    if (end == text + length)
        return kind;
    if (*end != '.')
        return -1;
    unsigned inner = (unsigned)(text + length - (end + 1));
    for (unsigned k = 0; k < kReplayObjectKindCount; k++) {
        const ReplayObjectKind& c = kReplayObjectKinds[k];
        if (c.hasInside && c.insideKind == kind && strlen(c.insideName) == inner && strncmp(c.insideName, end + 1, inner) == 0)
            return (int)k;
    }
    return -1;
}

}

void* ReplayResolveObject(const char* text, unsigned* kindOut, ReplayObjectTable& table, ReplayAllocFn alloc)
{
    const char* hash = strchr(text, '#');
    if (hash == nullptr)
        return nullptr;
    int kind = ReplayFindObjectKind(text, (unsigned)(hash - text));
    if (kind < 0)
        return nullptr;
    int result = ObjectTextKind(text, (unsigned)strlen(text));
    if (result < 0)
        result = kind;
    if (kindOut != nullptr)
        *kindOut = (unsigned)result;
    if (hash[1] == '?')
        return nullptr;
    unsigned ordinal = (unsigned)atoi(hash + 1);
    if (kReplayObjectKinds[kind].scope == RS_INDEX)
        return ordinal < 256 && g_game != nullptr ? &g_game->weapons[ordinal] : nullptr;
    void* p;
    if (kReplayObjectKinds[kind].polyCount != 0)
        return nullptr;
    if (kReplayObjectKinds[kind].table != RT_ORDER) {
        EnsureTable((unsigned)kind, alloc);
        p = table.Pointer((unsigned)kind, ordinal + 1);
    } else {
        p = table.Pointer((unsigned)kind, ordinal);
        if (p == nullptr && ordinal != 0 && alloc != nullptr) {
            p = alloc(kReplayObjectKinds[kind].size);
            memset(p, 0, kReplayObjectKinds[kind].size);
            if (ReplayVtable(kind) != nullptr)
                *(const void**)p = ReplayVtable(kind);
            table.Bind((unsigned)kind, ordinal, p);
        }
    }
    if (p != nullptr && result != kind)
        p = (unsigned char*)p + kReplayObjectKinds[result].insideOffset;
    return p;
}

bool ReplayRestoreObjectField(const ReplayObjectField& f, void* object, const char* value, ReplayObjectTable& table, ReplayAllocFn alloc)
{
    unsigned char* at = (unsigned char*)object + f.offset;
    switch (f.kind) {
    case RK_I32:
        *(int*)at = atoi(value);
        break;
    case RK_I16:
        *(short*)at = (short)atoi(value);
        break;
    case RK_U32:
        *(unsigned*)at = strtoul(value, nullptr, 10);
        break;
    case RK_U16:
        *(unsigned short*)at = (unsigned short)strtoul(value, nullptr, 10);
        break;
    case RK_F32:
        *(unsigned*)at = strncmp(value, "f32:0x", 6) == 0 ? ParseHex8(value + 6) : 0;
        break;
    case RK_U8:
        *at = (unsigned char)strtoul(value, nullptr, 10);
        break;
    case RK_F64:
        if (strncmp(value, "f64:0x", 6) == 0 && strlen(value) >= 22) {
            ((unsigned*)at)[1] = ParseHex8(value + 6);
            ((unsigned*)at)[0] = ParseHex8(value + 14);
        } else {
            ((unsigned*)at)[0] = 0;
            ((unsigned*)at)[1] = 0;
        }
        break;
    case RK_BYTES:
        memset(at, 0, f.size);
        if (strncmp(value, "x:", 2) == 0) {
            for (unsigned i = 0; i < f.size && strlen(value + 2 + i * 2) >= 2; i++) {
                const char* h = value + 2 + i * 2;
                at[i] = (unsigned char)((h[0] <= '9' ? h[0] - '0' : h[0] - 'a' + 10) << 4 | (h[1] <= '9' ? h[1] - '0' : h[1] - 'a' + 10));
            }
        }
        break;
    case RK_EMBED:
    case RK_UNITID:
    case RK_ARRAY:
        return false;
    case RK_CODE:
        return RestoreCode(f, value, (const void**)at);
    case RK_VECTOR:
        return false;
    case RK_FLAGS:
        *(unsigned*)at = strncmp(value, "0x", 2) == 0 ? ParseHex8(value + 2) : 0;
        break;
    case RK_POINT16: {
        int x = 0;
        int y = 0;
        sscanf(value, "(%d,%d)", &x, &y);
        ((short*)at)[0] = (short)x;
        ((short*)at)[1] = (short)y;
        break;
    }
    case RK_STRING: {
        memset(at, 0, f.size);
        unsigned n = 0;
        const char* p = value;
        if (*p == '"')
            p++;
        for (; *p != '\0' && *p != '"'; p++) {
            char c = *p;
            if (c == '\\') {
                p++;
                if (*p == 'n')
                    c = '\n';
                else if (*p == 'r')
                    c = '\r';
                else if (*p == 't')
                    c = '\t';
                else if (*p == 'x') {
                    unsigned v = 0;
                    for (int i = 1; i <= 2; i++) {
                        char d = p[i];
                        v = v << 4 | (unsigned)(d <= '9' ? d - '0' : d - 'a' + 10);
                    }
                    c = (char)v;
                    p += 2;
                } else
                    c = *p;
            }
            if (n < f.size)
                at[n++] = (unsigned char)c;
        }
        break;
    }
    case RK_WEAPON: {
        void* weapon = ReplayParseWeapon(value);
        if (weapon == nullptr && strcmp(value, "null") != 0)
            return false;
        *(unsigned*)at = weapon == nullptr ? 0u : (unsigned)((const char*)weapon - (const char*)SimBlockBase());
        break;
    }
    case RK_REF: {
        void* target = nullptr;
        if (strcmp(value, "null") != 0) {
            unsigned kind = 0;
            target = ReplayResolveObject(value, &kind, table, alloc);
            if (target == nullptr || !ReplayKindFits(f.target, kind))
                return false;
        }
        *(unsigned*)at = target == nullptr ? 0u : (unsigned)((const char*)target - (const char*)SimBlockBase());
        break;
    }
    default:
        return false;
    }
    return true;
}

bool ReplayParseKey(const char* key, unsigned* kind, unsigned* ordinal, const char** field)
{
    const char* hash = strchr(key, '#');
    const char* dot = strrchr(key, '.');
    if (hash == nullptr || dot == nullptr || dot < hash)
        return false;
    int k = ObjectTextKind(key, (unsigned)(dot - key));
    if (k < 0)
        return false;
    *kind = (unsigned)k;
    *ordinal = (unsigned)atoi(hash + 1);
    *field = dot + 1;
    return true;
}

void ReplayObjectFieldKey(ReplayText& key, ReplayObjectTable& table, bool assign, unsigned kind, const void* object, const ReplayObjectField& f)
{
    key.Clear();
    ReplayAddObjectName(key, table, kind, object, assign);
    key.AddChar('.');
    key.Add(f.name);
}

namespace {

enum { MAX_WALK_FIELDS = 64, MAX_DEFERRED = 512 };

bool Walkable(const ReplayObjectField& f)
{
    if (f.kind == RK_VECTOR)
        return kReplayVectors[f.array - 1].mode == RV_OBJECT;
    return f.kind == RK_REF || f.kind == RK_EMBED || f.kind == RK_UNITID || f.kind == RK_WEAPON;
}

unsigned WalkFields(const ReplaySlotSets& sets, unsigned short* out)
{
    unsigned n = 0;
    for (unsigned pass = 0; pass < 2; pass++) {
        const unsigned short* set = pass == 0 ? sets.objReads : sets.objWrites;
        unsigned count = pass == 0 ? sets.objReadCount : sets.objWriteCount;
        for (unsigned i = 0; i < count; i++) {
            if (!Walkable(kReplayObjectFields[set[i]]))
                continue;
            unsigned at = 0;
            while (at < n && out[at] != set[i])
                at++;
            if (at < n)
                continue;
            if (n == MAX_WALK_FIELDS)
                abort();
            out[n++] = set[i];
        }
    }
    for (unsigned a = 1; a < n; a++) {
        for (unsigned b = a; b > 0 && out[b - 1] > out[b]; b--) {
            unsigned short t = out[b];
            out[b] = out[b - 1];
            out[b - 1] = t;
        }
    }
    return n;
}

bool InWalkFields(const unsigned short* fields, unsigned count, unsigned field)
{
    for (unsigned i = 0; i < count; i++) {
        if (fields[i] == field)
            return true;
    }
    return false;
}

void Visit(ReplayReach& reach, ReplayObjectTable& table, bool assign, unsigned char kind, void* object, const unsigned short* limit, unsigned limitCount)
{
    if (object == nullptr || kReplayObjectKinds[kind].scope == RS_VALUE)
        return;
    unsigned actual = ReplayConcreteKind(kind, object);
    if (actual == kReplayObjectKindCount)
        return;
    kind = (unsigned char)actual;
    unsigned slot = PointerHash(object) % REPLAY_REACH_HASH;
    for (; reach.slots[slot] >= 0; slot = (slot + 1) % REPLAY_REACH_HASH) {
        if (reach.object[reach.slots[slot]] == object && reach.kind[reach.slots[slot]] == kind)
            return;
    }
    if (reach.count == REPLAY_MAX_OBJECTS)
        abort();
    reach.slots[slot] = (short)reach.count;
    if (assign && kReplayObjectKinds[kind].scope == RS_STATE && !ReplayInsideTable(kind, object))
        table.Name(kind, object);
    reach.kind[reach.count] = kind;
    reach.object[reach.count] = object;
    reach.limit[reach.count] = limit;
    reach.limitCount[reach.count] = (unsigned short)limitCount;
    reach.count++;
}

void VisitRange(ReplayReach& reach, ReplayObjectTable& table, bool assign, unsigned char kind, void* begin, void* end, const unsigned short* limit, unsigned limitCount)
{
    unsigned stride = kReplayObjectKinds[kind].size;
    unsigned char* b = (unsigned char*)begin;
    unsigned char* e = (unsigned char*)end;
    if (b == nullptr || e < b || (unsigned)(e - b) % stride != 0 || (unsigned)(e - b) / stride >= REPLAY_MAX_OBJECTS) {
        Visit(reach, table, assign, kind, begin, limit, limitCount);
        Visit(reach, table, assign, kind, end, limit, limitCount);
        return;
    }
    for (unsigned char* u = b; u <= e; u += stride)
        Visit(reach, table, assign, kind, u, limit, limitCount);
}

struct Deferred {
    unsigned count;
    unsigned done;
    void* object[MAX_DEFERRED];
    unsigned short field[MAX_DEFERRED];
};

void Expand(ReplayReach& reach, ReplayObjectTable& table, bool assign, unsigned at, const unsigned short* fields, unsigned fieldCount, Deferred& deferred)
{
    const unsigned short* list = reach.limit[at] != nullptr ? reach.limit[at] : fields;
    unsigned count = reach.limit[at] != nullptr ? reach.limitCount[at] : fieldCount;
    const unsigned short* limit = reach.limit[at];
    unsigned limitCount = reach.limitCount[at];
    unsigned char* base = (unsigned char*)reach.object[at];
    for (unsigned i = 0; i < count; i++) {
        const ReplayObjectField& field = kReplayObjectFields[list[i]];
        if (field.object != reach.kind[at] || !Walkable(field))
            continue;
        if (field.kind == RK_EMBED) {
            Visit(reach, table, assign, field.target, base + field.offset, limit, limitCount);
            continue;
        }
        if (field.kind == RK_VECTOR) {
            VectorView view = ViewOf(field, base);
            for (unsigned e = 0; e < view.length; e++)
                Visit(reach, table, assign, field.target, BlockAt(((const unsigned*)view.data)[e]), limit, limitCount);
            continue;
        }
        if (field.kind == RK_UNITID) {
            if (field.condField != 0 && (int)*(const short*)(base + kReplayObjectFields[field.condField - 1u].offset) != field.condValue)
                continue;
            unsigned id = *(const unsigned short*)(base + field.offset);
            unsigned char* units;
            unsigned poolCount;
            unsigned stride;
            if (id == 0 || !TableSpan(field.target, &units, &poolCount, &stride) || id >= poolCount || deferred.count == MAX_DEFERRED)
                continue;
            deferred.object[deferred.count] = units + id * stride;
            deferred.field[deferred.count++] = list[i];
            continue;
        }
        void* target = BlockAt(*(const unsigned*)(base + field.offset));
        if (field.kind == RK_REF && field.rangeEnd != 0 && InWalkFields(list, count, field.rangeEnd - 1u)) {
            const ReplayObjectField& end = kReplayObjectFields[field.rangeEnd - 1u];
            VisitRange(reach, table, assign, field.target, target, BlockAt(*(const unsigned*)(base + end.offset)), limit, limitCount);
        } else if (field.kind == RK_REF && field.rangeBegin != 0 && InWalkFields(list, count, field.rangeBegin - 1u)) {
            continue;
        } else {
            Visit(reach, table, assign, field.target, target, limit, limitCount);
        }
    }
}

}

bool ReplayInReach(const ReplayReach& reach, unsigned at, unsigned fieldIndex)
{
    if (reach.limit[at] == nullptr)
        return true;
    for (unsigned i = 0; i < reach.limitCount[at]; i++) {
        if (reach.limit[at][i] == fieldIndex)
            return true;
    }
    return false;
}

void ReplayWalk(ReplayReach& reach, ReplayObjectTable& table, bool assign, const ReplaySlotSets& sets, const unsigned char* rootKinds, void* const* roots, unsigned rootCount,
                const unsigned short* rootLimit, unsigned rootLimitCount)
{
    reach.count = 0;
    memset(reach.slots, 0xff, sizeof reach.slots);
    for (unsigned i = 0; i < rootCount; i++)
        Visit(reach, table, assign, rootKinds[i], roots[i], rootLimit, rootLimitCount);
    if (sets.allOf != 0) {
        unsigned kind = sets.allOf - 1u;
        unsigned char* base;
        unsigned count;
        unsigned stride;
        if (TableSpan(kind, &base, &count, &stride)) {
            for (unsigned i = 0; i < count; i++)
                Visit(reach, table, assign, (unsigned char)kind, base + i * stride, nullptr, 0);
        }
    }
    unsigned short fields[MAX_WALK_FIELDS];
    unsigned fieldCount = WalkFields(sets, fields);
    static Deferred deferred;
    deferred.count = 0;
    deferred.done = 0;
    unsigned at = 0;
    for (;;) {
        for (; at < reach.count; at++)
            Expand(reach, table, assign, at, fields, fieldCount, deferred);
        if (deferred.done == deferred.count)
            break;
        for (; deferred.done < deferred.count; deferred.done++) {
            const ReplayObjectField& field = kReplayObjectFields[deferred.field[deferred.done]];
            Visit(reach, table, assign, field.target, deferred.object[deferred.done], kReplayReachFields + field.reachFirst, field.reachCount);
        }
    }
}

void ReplayCaptureObjects(ReplayText& out, ReplayKnown& known, ReplayObjectTable& table, const ReplayReach& reach, const unsigned short* set, unsigned count, char tag)
{
    ReplayText key;
    ReplayText value;
    for (unsigned at = 0; at < reach.count; at++) {
        for (unsigned i = 0; i < count; i++) {
            const ReplayObjectField& f = kReplayObjectFields[set[i]];
            if (f.object != reach.kind[at] || f.kind == RK_EMBED || f.kind == RK_UNITID || !ReplayInReach(reach, at, set[i]))
                continue;
            ReplayObjectFieldKey(key, table, true, reach.kind[at], reach.object[at], f);
            ReplayEmitField(out, known, table, tag, key.Data(), f, reach.object[at], value);
        }
    }
    key.Free();
    value.Free();
}

ReplayObjectField ReplayFieldView(const ReplayField& f)
{
    ReplayObjectField view = {};
    view.name = f.name;
    view.offset = f.offset;
    view.size = f.size;
    view.kind = f.kind;
    view.array = f.array;
    return view;
}

namespace {

void EmitLine(ReplayText& out, char tag, const char* key, const char* value)
{
    out.Add("  ");
    out.AddChar(tag);
    out.AddChar(' ');
    out.Add(key);
    out.AddChar(' ');
    out.Add(value);
    out.AddChar('\n');
}

unsigned BracketLength(const char* text)
{
    return text != nullptr && text[0] == '[' ? (unsigned)strtoul(text + 1, nullptr, 10) : 0;
}

void EmitVector(ReplayText& out, ReplayKnown& known, ReplayObjectTable& table, char tag, const char* key, const ReplayObjectField& f, const void* object, const char* now)
{
    const ReplayVectorSpec& spec = kReplayVectors[f.array - 1];
    if (spec.mode == RV_SCALAR) {
        EmitArray(out, known, key, tag, now, spec.size);
        return;
    }
    unsigned before = BracketLength(known.Get(key));
    if (known.Changed(key, now))
        EmitLine(out, tag, key, now);
    if (spec.mode == RV_OBJECT)
        return;
    VectorView view = ViewOf(f, object);
    ReplayText value;
    for (unsigned i = 0; i < view.length; i++) {
        for (unsigned k = 0; k < kReplayObjectFieldCount; k++) {
            const ReplayObjectField& ef = kReplayObjectFields[k];
            if (ef.object != spec.kind)
                continue;
            char sub[160];
            sprintf(sub, "%s[%u].%s", key, i, ef.name);
            ReplayFormatObjectField(ef, view.data + i * view.size, table, true, value);
            bool fresh = i >= before;
            if (fresh)
                known.Set(sub, value.Data());
            if (fresh || known.Changed(sub, value.Data()))
                EmitLine(out, tag, sub, value.Data());
        }
    }
    value.Free();
}

}

void ReplayEmitField(ReplayText& out, ReplayKnown& known, ReplayObjectTable& table, char tag, const char* key, const ReplayObjectField& f, const void* object, ReplayText& scratch)
{
    ReplayFormatObjectField(f, object, table, true, scratch);
    if (f.kind == RK_ARRAY) {
        EmitArray(out, known, key, tag, scratch.Data(), kReplayArrays[f.array - 1].elem);
        return;
    }
    if (f.kind == RK_VECTOR) {
        EmitVector(out, known, table, tag, key, f, object, scratch.Data());
        return;
    }
    if (known.Changed(key, scratch.Data())) {
        out.Add("  ");
        out.AddChar(tag);
        out.AddChar(' ');
        out.Add(key);
        out.AddChar(' ');
        out.Add(scratch.Data());
        out.AddChar('\n');
    }
}

bool ReplayApplyArray(const ReplayObjectField& f, void* object, const char* key, const char* value, ReplayKnown& known, bool memory, ReplayAllocFn alloc)
{
    const ReplayArraySpec& a = kReplayArrays[f.array - 1];
    unsigned char* at = memory ? (unsigned char*)object + f.offset : nullptr;
    if (strcmp(value, "null") == 0) {
        if (!a.ref)
            return false;
        if (memory)
            *(unsigned*)at = 0;
        known.Set(key, "null");
        return true;
    }
    const char* p = value;
    unsigned count;
    if (*p == '[') {
        char* end;
        count = (unsigned)strtoul(p + 1, &end, 10);
        if (*end != ']' || count > MAX_ARRAY_ELEMENTS || (!a.ref && count * a.elem > a.capacity))
            return false;
        p = end + 1;
        if (*p == ' ')
            p++;
        ReplayText zero;
        zero.AddChar('[');
        zero.AddUnsigned(count);
        zero.AddChar(']');
        for (unsigned i = 0; i < count * a.elem * 2; i++)
            zero.AddChar('0');
        known.Set(key, zero.Data());
        zero.Free();
        if (memory) {
            if (a.ref) {
                if (count != 0 && alloc == nullptr)
                    return false;
                unsigned char* mem = count == 0 ? nullptr : (unsigned char*)alloc(count * a.elem);
                if (mem != nullptr)
                    memset(mem, 0, count * a.elem);
                *(unsigned*)at = mem == nullptr ? 0u : (unsigned)(mem - (unsigned char*)SimBlockBase());
            } else {
                memset(at, 0, count * a.elem);
            }
        }
    } else {
        const char* old = known.Get(key);
        if (old == nullptr || old[0] != '[')
            return false;
        count = ArrayCount(old);
    }
    char* text = known.Edit(key);
    char* hex = text + (ArrayHex(text) - text);
    unsigned char* data = nullptr;
    if (memory) {
        data = a.ref ? BlockAt(*(const unsigned*)at) : at;
        if (data == nullptr && count != 0)
            return false;
    }
    unsigned width = a.elem * 2;
    while (*p != '\0') {
        if (*p == ' ') {
            p++;
            continue;
        }
        char* end;
        unsigned long index = strtoul(p, &end, 10);
        if (*end != ':')
            return false;
        p = end + 1;
        unsigned length = 0;
        while ((p[length] >= '0' && p[length] <= '9') || (p[length] >= 'a' && p[length] <= 'f'))
            length++;
        if (length % width != 0 || (unsigned long long)index * width + length > (unsigned long long)count * width)
            return false;
        memcpy(hex + index * width, p, length);
        if (memory) {
            for (unsigned i = 0; i < length / 2; i++)
                data[index * a.elem + i] = (unsigned char)(HexDigit(p[i * 2]) << 4 | HexDigit(p[i * 2 + 1]));
        }
        p += length;
    }
    return true;
}

bool ReplayParseElementKey(const char* key, char* prefix, unsigned size, unsigned* index, const char** sub)
{
    const char* open = strchr(key, '[');
    if (open == nullptr || open[1] < '0' || open[1] > '9')
        return false;
    char* end;
    unsigned long at = strtoul(open + 1, &end, 10);
    if (end[0] != ']' || end[1] != '.' || (unsigned)(open - key) >= size)
        return false;
    memcpy(prefix, key, (unsigned)(open - key));
    prefix[open - key] = '\0';
    *index = (unsigned)at;
    *sub = end + 2;
    return true;
}

namespace {

bool ParseBracket(const char** p, unsigned* length, unsigned* capacity)
{
    char* end;
    *length = (unsigned)strtoul(*p + 1, &end, 10);
    if (*end != '/')
        return false;
    *capacity = (unsigned)strtoul(end + 1, &end, 10);
    if (*end != ']' || *capacity < *length || *capacity > MAX_ARRAY_ELEMENTS)
        return false;
    *p = end + 1;
    return true;
}

void StoreWords(unsigned char* at, const unsigned char* data, unsigned length, unsigned capacity, unsigned size)
{
    unsigned* words = (unsigned*)(at + 4);
    if (data == nullptr) {
        words[0] = words[1] = words[2] = 0;
        return;
    }
    unsigned first = (unsigned)(data - (const unsigned char*)SimBlockBase());
    words[0] = first;
    words[1] = first + length * size;
    words[2] = first + capacity * size;
}

}

bool ReplayApplyVector(const ReplayObjectField& f, void* object, const char* key, const char* value, ReplayKnown& known, bool memory, ReplayObjectTable& table, ReplayAllocFn alloc)
{
    const ReplayVectorSpec& spec = kReplayVectors[f.array - 1];
    unsigned char* at = (unsigned char*)object + f.offset;
    const char* p = value;
    unsigned length;
    unsigned capacity;
    if (*p == '[') {
        if (!ParseBracket(&p, &length, &capacity))
            return false;
        if (*p == ' ')
            p++;
        ReplayText canonical;
        AddBracket(canonical, length, capacity);
        if (spec.mode == RV_OBJECT) {
            canonical.Add(*p == '\0' ? "" : " ");
            canonical.Add(p);
        } else if (spec.mode == RV_SCALAR) {
            for (unsigned i = 0; i < length * spec.size * 2; i++)
                canonical.AddChar('0');
        }
        known.Set(key, canonical.Data());
        canonical.Free();
        VectorView old = {};
        if (memory) {
            old = ViewOf(f, object);
            unsigned char* data = old.data;
            if (data == nullptr || old.capacity != capacity) {
                if (capacity != 0 && alloc == nullptr)
                    return false;
                data = capacity == 0 ? nullptr : (unsigned char*)alloc(capacity * spec.size);
                if (data != nullptr)
                    memset(data, 0, capacity * spec.size);
                if (data != nullptr && old.data != nullptr && spec.mode == RV_VALUE)
                    memcpy(data, old.data, (old.length < length ? old.length : length) * spec.size);
            } else if (spec.mode == RV_SCALAR) {
                memset(data, 0, capacity * spec.size);
            }
            StoreWords(at, data, length, capacity, spec.size);
        }
        if (spec.mode == RV_OBJECT) {
            VectorView view = memory ? ViewOf(f, object) : VectorView();
            for (unsigned i = 0; i < length; i++) {
                char token[96];
                unsigned n = 0;
                while (*p != '\0' && *p != ',' && n + 1 < sizeof token)
                    token[n++] = *p++;
                token[n] = '\0';
                if (*p == ',')
                    p++;
                if (!memory)
                    continue;
                void* target = nullptr;
                if (strcmp(token, "null") != 0) {
                    unsigned found = 0;
                    target = ReplayResolveObject(token, &found, table, alloc);
                    if (target == nullptr || !ReplayKindFits(spec.kind, found))
                        return false;
                }
                ((unsigned*)view.data)[i] = target == nullptr ? 0u : (unsigned)((const char*)target - (const char*)SimBlockBase());
            }
            return true;
        }
        if (spec.mode == RV_VALUE)
            return true;
    } else {
        const char* old = known.Get(key);
        if (spec.mode != RV_SCALAR || old == nullptr || old[0] != '[')
            return false;
        p = old;
        if (!ParseBracket(&p, &length, &capacity))
            return false;
        p = value;
    }
    char* text = known.Edit(key);
    char* hex = strchr(text, ']') + 1;
    unsigned char* data = memory ? ViewOf(f, object).data : nullptr;
    if (memory && data == nullptr && length != 0)
        return false;
    unsigned width = spec.size * 2;
    while (*p != '\0') {
        if (*p == ' ') {
            p++;
            continue;
        }
        char* end;
        unsigned long index = strtoul(p, &end, 10);
        if (*end != ':')
            return false;
        p = end + 1;
        unsigned run = 0;
        while ((p[run] >= '0' && p[run] <= '9') || (p[run] >= 'a' && p[run] <= 'f'))
            run++;
        if (run % width != 0 || (unsigned long long)index * width + run > (unsigned long long)length * width)
            return false;
        memcpy(hex + index * width, p, run);
        if (memory) {
            for (unsigned i = 0; i < run / 2; i++)
                data[index * spec.size + i] = (unsigned char)(HexDigit(p[i * 2]) << 4 | HexDigit(p[i * 2 + 1]));
        }
        p += run;
    }
    return true;
}

bool ReplayApplyVectorElement(const ReplayObjectField& f, void* object, unsigned index, const char* sub, const char* fullKey, const char* value, ReplayKnown& known, bool memory, ReplayObjectTable& table, ReplayAllocFn alloc)
{
    const ReplayVectorSpec& spec = kReplayVectors[f.array - 1];
    if (spec.mode != RV_VALUE)
        return false;
    known.Set(fullKey, value);
    if (!memory)
        return true;
    VectorView view = ViewOf(f, object);
    const ReplayObjectField* ef = ReplayFindObjectField(spec.kind, sub);
    if (ef == nullptr || index >= view.length)
        return false;
    return ReplayRestoreObjectField(*ef, view.data + index * view.size, value, table, alloc);
}

void ReplayCompareVector(const ReplayObjectField& f, const void* object, const char* key, const ReplayKnown& known, ReplayObjectTable& table, ReplayReportFn report, ReplayHashFn hash)
{
    const ReplayVectorSpec& spec = kReplayVectors[f.array - 1];
    VectorView view = ViewOf(f, object);
    ReplayText now;
    FormatVector(f, object, table, false, now);
    hash(now.Data());
    const char* recorded = known.Get(key);
    if (recorded == nullptr) {
        report(key, "no value", now.Data());
        now.Free();
        return;
    }
    const char* p = recorded;
    unsigned length = 0;
    unsigned capacity = 0;
    if (!ParseBracket(&p, &length, &capacity)) {
        report(key, recorded, now.Data());
        now.Free();
        return;
    }
    char name[200];
    char a[48];
    char b[48];
    if (length != view.length) {
        sprintf(name, "%s.length", key);
        sprintf(a, "%u", length);
        sprintf(b, "%u", view.length);
        report(name, a, b);
    }
    if (capacity != view.capacity) {
        sprintf(name, "%s.capacity", key);
        sprintf(a, "%u", capacity);
        sprintf(b, "%u", view.capacity);
        report(name, a, b);
    }
    unsigned common = length < view.length ? length : view.length;
    if (spec.mode == RV_VALUE) {
        ReplayText value;
        for (unsigned i = 0; i < common; i++) {
            for (unsigned k = 0; k < kReplayObjectFieldCount; k++) {
                const ReplayObjectField& ef = kReplayObjectFields[k];
                if (ef.object != spec.kind)
                    continue;
                sprintf(name, "%s[%u].%s", key, i, ef.name);
                ReplayFormatObjectField(ef, view.data + i * view.size, table, false, value);
                hash(value.Data());
                const char* want = known.Get(name);
                if (want == nullptr)
                    report(name, "no value", value.Data());
                else if (strcmp(want, value.Data()) != 0)
                    report(name, want, value.Data());
            }
        }
        value.Free();
    } else if (spec.mode == RV_SCALAR) {
        const char* want = p;
        const char* have = strchr(now.Data(), ']') + 1;
        unsigned width = spec.size * 2;
        for (unsigned i = 0; i < common; i++) {
            if (strncmp(want + i * width, have + i * width, width) != 0) {
                sprintf(name, "%s[%u]", key, i);
                char x[24];
                char y[24];
                memcpy(x, want + i * width, width);
                x[width] = '\0';
                memcpy(y, have + i * width, width);
                y[width] = '\0';
                report(name, x, y);
            }
        }
    } else {
        if (*p == ' ')
            p++;
        const char* have = strchr(now.Data(), ']') + 1;
        if (*have == ' ')
            have++;
        for (unsigned i = 0; i < common; i++) {
            char x[96];
            char y[96];
            unsigned n = 0;
            while (*p != '\0' && *p != ',' && n + 1 < sizeof x)
                x[n++] = *p++;
            x[n] = '\0';
            if (*p == ',')
                p++;
            n = 0;
            while (*have != '\0' && *have != ',' && n + 1 < sizeof y)
                y[n++] = *have++;
            y[n] = '\0';
            if (*have == ',')
                have++;
            if (strcmp(x, y) != 0) {
                sprintf(name, "%s[%u]", key, i);
                report(name, x, y);
            }
        }
    }
    now.Free();
}

void ReplayArrayDiff(const char* recorded, const char* replayed, ReplayText& out)
{
    bool a = recorded[0] == '[';
    bool b = replayed[0] == '[';
    if (!a || !b) {
        out.Add(a ? "recorded an array, replayed " : "recorded ");
        out.Add(a ? replayed : recorded);
        out.Add(a ? "" : ", replayed an array");
        return;
    }
    unsigned countA = ArrayCount(recorded);
    unsigned countB = ArrayCount(replayed);
    const char* hexA = ArrayHex(recorded);
    const char* hexB = ArrayHex(replayed);
    if (countA != countB) {
        char text[96];
        sprintf(text, "recorded %u elements, replayed %u", countA, countB);
        out.Add(text);
        return;
    }
    unsigned length = (unsigned)strlen(hexA);
    unsigned first = length;
    unsigned differing = 0;
    for (unsigned i = 0; i + 1 < length; i += 2) {
        if (hexA[i] != hexB[i] || hexA[i + 1] != hexB[i + 1]) {
            if (first == length)
                first = i / 2;
            differing++;
        }
    }
    char text[160];
    sprintf(text, "byte %u of %u: recorded %.2s, replayed %.2s, %u bytes differ", first, length / 2, hexA + first * 2, hexB + first * 2, differing);
    out.Add(text);
}

bool ReplayIsArrayText(const char* text)
{
    return text[0] == '[' && text[1] >= '0' && text[1] <= '9';
}

void ReplayAddStruct(ReplayText& t, const char* tag, const char* name, unsigned kind, const void* p, ReplayObjectTable& table)
{
    if (p == nullptr) {
        t.Add("  ");
        t.Add(tag);
        t.AddChar(' ');
        t.Add(name);
        t.Add(" null\n");
        return;
    }
    ReplayText value;
    for (unsigned i = 0; i < kReplayObjectFieldCount; i++) {
        const ReplayObjectField& f = kReplayObjectFields[i];
        if (f.object != kind)
            continue;
        ReplayFormatObjectField(f, p, table, true, value);
        t.Add("  ");
        t.Add(tag);
        t.AddChar(' ');
        t.Add(name);
        t.AddChar('.');
        t.Add(f.name);
        t.AddChar(' ');
        t.Add(value.Data());
        t.AddChar('\n');
    }
    value.Free();
}

void ReplayAddStructs(ReplayText& t, const char* tag, const char* name, unsigned kind, const void* p, int count, ReplayObjectTable& table)
{
    if (p == nullptr || count <= 0) {
        t.Add("  ");
        t.Add(tag);
        t.AddChar(' ');
        t.Add(name);
        t.Add(p == nullptr ? " null\n" : " []\n");
        return;
    }
    for (int i = 0; i < count; i++) {
        char element[96];
        sprintf(element, "%.80s[%d]", name, i);
        ReplayAddStruct(t, tag, element, kind, (const unsigned char*)p + (unsigned)i * kReplayObjectKinds[kind].size, table);
    }
}

void ReplayAddValue(ReplayText& t, unsigned kind, const void* p, ReplayObjectTable& table)
{
    ReplayText value;
    t.AddChar('(');
    bool first = true;
    for (unsigned i = 0; i < kReplayObjectFieldCount; i++) {
        const ReplayObjectField& f = kReplayObjectFields[i];
        if (f.object != kind)
            continue;
        if (!first)
            t.AddChar(',');
        first = false;
        ReplayFormatObjectField(f, p, table, true, value);
        t.Add(value.Data());
    }
    t.AddChar(')');
    value.Free();
}

bool ReplayParseValue(unsigned kind, const char* text, void* p, ReplayObjectTable& table, ReplayAllocFn alloc)
{
    if (text[0] != '(')
        return false;
    const char* q = text + 1;
    for (unsigned i = 0; i < kReplayObjectFieldCount; i++) {
        const ReplayObjectField& f = kReplayObjectFields[i];
        if (f.object != kind)
            continue;
        char piece[256];
        unsigned n = 0;
        int depth = 0;
        bool quoted = false;
        for (; *q != '\0'; q++) {
            if (quoted) {
                if (*q == '\\' && q[1] != '\0') {
                    if (n + 2 < sizeof piece) {
                        piece[n++] = *q;
                        piece[n++] = q[1];
                    }
                    q++;
                    continue;
                }
                quoted = *q != '"';
            } else if (*q == '"') {
                quoted = true;
            } else if (*q == '(') {
                depth++;
            } else if (*q == ')' && depth == 0) {
                break;
            } else if (*q == ')') {
                depth--;
            } else if (*q == ',' && depth == 0) {
                break;
            }
            if (n + 1 < sizeof piece)
                piece[n++] = *q;
        }
        piece[n] = '\0';
        if (!ReplayRestoreObjectField(f, p, piece, table, alloc))
            return false;
        bool last = true;
        for (unsigned j = i + 1; j < kReplayObjectFieldCount; j++)
            last = last && kReplayObjectFields[j].object != kind;
        if (last)
            break;
        if (*q != ',')
            return false;
        q++;
    }
    return *q == ')' && q[1] == '\0';
}

namespace {

enum { MAX_ACTIVE_CALLBACKS = 8 };

ReplayCallbackScope* s_scopes[MAX_ACTIVE_CALLBACKS];
unsigned s_callbackDepth;

ReplayCallbackScope* FindScope(const void* object)
{
    for (unsigned i = 0; i < MAX_ACTIVE_CALLBACKS; i++) {
        if (s_scopes[i] != nullptr && s_scopes[i]->Object() == object)
            return s_scopes[i];
    }
    return nullptr;
}

}

ReplayCallbackScope::ReplayCallbackScope() : active(false), callback(0), object(nullptr), original(nullptr), ordinal(0), table(nullptr)
{
}

ReplayCallbackScope::~ReplayCallbackScope()
{
    Stop();
    lines.Free();
}

void ReplayCallbackScope::Start(unsigned id, void* target, unsigned number, ReplayObjectTable& names, const void* thunkVtable)
{
    if (active || target == nullptr)
        return;
    unsigned slot = 0;
    while (slot < MAX_ACTIVE_CALLBACKS && s_scopes[slot] != nullptr)
        slot++;
    if (slot == MAX_ACTIVE_CALLBACKS || FindScope(target) != nullptr)
        abort();
    active = true;
    callback = id;
    object = target;
    ordinal = number;
    table = &names;
    original = *(const void* const*)target;
    *(const void**)target = thunkVtable;
    s_scopes[slot] = this;
}

void ReplayCallbackScope::Stop()
{
    if (!active)
        return;
    active = false;
    for (unsigned i = 0; i < MAX_ACTIVE_CALLBACKS; i++) {
        if (s_scopes[i] == this)
            s_scopes[i] = nullptr;
    }
    *(const void**)object = original;
}

ReplayCallbackCall::ReplayCallbackCall(const void* target, unsigned method) : scope(nullptr)
{
    ReplayCallbackScope* found = FindScope(target);
    if (found == nullptr) {
        abort();
        return;
    }
    scope = found;
    if (s_callbackDepth != 0)
        ReplayFail("a callback made inside a callback of an import is not recorded");
    s_callbackDepth++;
    const ReplayCallbackType& type = kReplayCallbacks[found->callback];
    text.Add("\n    c ");
    text.Add(type.handle);
    text.AddChar('#');
    text.AddUnsigned(found->ordinal);
    text.AddChar('.');
    text.Add(type.methods[method]);
}

ReplayCallbackCall::~ReplayCallbackCall()
{
    scope->lines.AddText(text);
    s_callbackDepth--;
    text.Free();
}

const void* ReplayCallbackCall::Original(unsigned slot) const
{
    return ((const void* const*)scope->original)[slot];
}

const void* ReplayVptrOf(const void* object)
{
    return *(const void* const*)object;
}

unsigned ReplayCallbackDepth()
{
    return s_callbackDepth;
}
