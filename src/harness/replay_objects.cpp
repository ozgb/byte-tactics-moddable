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

bool TableSpan(unsigned table, unsigned char** base, unsigned* count, unsigned* stride)
{
    if (g_game == nullptr)
        return false;
    if (table == RT_UNITS) {
        *base = (unsigned char*)g_game->units.get();
        *count = g_game->poolCount;
        *stride = (unsigned)sizeof(Unit);
    } else if (table == RT_PLAYERS) {
        *base = (unsigned char*)g_game->players;
        *count = 10;
        *stride = (unsigned)sizeof(Player);
    } else {
        return false;
    }
    return *base != nullptr;
}

int TableIndex(unsigned table, const void* p)
{
    unsigned char* base;
    unsigned count;
    unsigned stride;
    if (p == nullptr || !TableSpan(table, &base, &count, &stride))
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
    int at = TableIndex(kReplayObjectKinds[k.insideKind].table, (const unsigned char*)p - k.insideOffset);
    if (at < 0)
        return false;
    *container = k.insideKind;
    *index = at;
    return true;
}

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
        int at = TableIndex(kReplayObjectKinds[kind].table, p);
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
        if (ordinal == 0 || !TableSpan(kReplayObjectKinds[kind].table, &base, &count, &stride) || ordinal > count)
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
    if (kReplayObjectKinds[kind].table != RT_ORDER) {
        if (kReplayObjectKinds[kind].table == RT_UNITS && g_game != nullptr && g_game->units.get() == nullptr && alloc != nullptr) {
            g_game->poolCount = REPLAY_UNIT_POOL;
            g_game->units = (Unit*)alloc(REPLAY_UNIT_POOL * (unsigned)sizeof(Unit));
        }
        p = table.Pointer((unsigned)kind, ordinal + 1);
    } else {
        p = table.Pointer((unsigned)kind, ordinal);
        if (p == nullptr && ordinal != 0 && alloc != nullptr) {
            p = alloc(kReplayObjectKinds[kind].size);
            memset(p, 0, kReplayObjectKinds[kind].size);
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
            if (target == nullptr || kind != f.target)
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

enum { MAX_WALK_FIELDS = 64 };

unsigned WalkFields(const ReplaySlotSets& sets, unsigned short* out)
{
    unsigned n = 0;
    for (unsigned pass = 0; pass < 2; pass++) {
        const unsigned short* set = pass == 0 ? sets.objReads : sets.objWrites;
        unsigned count = pass == 0 ? sets.objReadCount : sets.objWriteCount;
        for (unsigned i = 0; i < count; i++) {
            unsigned kind = kReplayObjectFields[set[i]].kind;
            if (kind != RK_REF && kind != RK_EMBED)
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

void Visit(ReplayReach& reach, ReplayObjectTable& table, bool assign, unsigned char kind, void* object)
{
    if (object == nullptr || kReplayObjectKinds[kind].scope != RS_STATE)
        return;
    unsigned slot = PointerHash(object) % REPLAY_REACH_HASH;
    for (; reach.slots[slot] >= 0; slot = (slot + 1) % REPLAY_REACH_HASH) {
        if (reach.object[reach.slots[slot]] == object && reach.kind[reach.slots[slot]] == kind)
            return;
    }
    if (reach.count == REPLAY_MAX_OBJECTS)
        abort();
    reach.slots[slot] = (short)reach.count;
    if (assign && !ReplayInsideTable(kind, object))
        table.Name(kind, object);
    reach.kind[reach.count] = kind;
    reach.object[reach.count] = object;
    reach.count++;
}

void VisitRange(ReplayReach& reach, ReplayObjectTable& table, bool assign, unsigned char kind, void* begin, void* end)
{
    unsigned stride = kReplayObjectKinds[kind].size;
    unsigned char* b = (unsigned char*)begin;
    unsigned char* e = (unsigned char*)end;
    if (b == nullptr || e < b || (unsigned)(e - b) % stride != 0 || (unsigned)(e - b) / stride >= REPLAY_MAX_OBJECTS) {
        Visit(reach, table, assign, kind, begin);
        Visit(reach, table, assign, kind, end);
        return;
    }
    for (unsigned char* u = b; u <= e; u += stride)
        Visit(reach, table, assign, kind, u);
}

}

void ReplayWalk(ReplayReach& reach, ReplayObjectTable& table, bool assign, const ReplaySlotSets& sets, const unsigned char* rootKinds, void* const* roots, unsigned rootCount)
{
    reach.count = 0;
    memset(reach.slots, 0xff, sizeof reach.slots);
    for (unsigned i = 0; i < rootCount; i++)
        Visit(reach, table, assign, rootKinds[i], roots[i]);
    if (sets.allOf != 0) {
        unsigned kind = sets.allOf - 1u;
        unsigned char* base;
        unsigned count;
        unsigned stride;
        if (TableSpan(kReplayObjectKinds[kind].table, &base, &count, &stride)) {
            for (unsigned i = 0; i < count; i++)
                Visit(reach, table, assign, (unsigned char)kind, base + i * stride);
        }
    }
    unsigned short fields[MAX_WALK_FIELDS];
    unsigned fieldCount = WalkFields(sets, fields);
    for (unsigned at = 0; at < reach.count; at++) {
        for (unsigned i = 0; i < fieldCount; i++) {
            const ReplayObjectField& field = kReplayObjectFields[fields[i]];
            if (field.object != reach.kind[at])
                continue;
            if (field.kind == RK_EMBED) {
                Visit(reach, table, assign, field.target, (unsigned char*)reach.object[at] + field.offset);
                continue;
            }
            void* target = BlockAt(*(const unsigned*)((const unsigned char*)reach.object[at] + field.offset));
            if (field.rangeEnd != 0 && InWalkFields(fields, fieldCount, field.rangeEnd - 1u)) {
                const ReplayObjectField& end = kReplayObjectFields[field.rangeEnd - 1u];
                VisitRange(reach, table, assign, field.target, target, BlockAt(*(const unsigned*)((const unsigned char*)reach.object[at] + end.offset)));
            } else if (field.rangeBegin != 0 && InWalkFields(fields, fieldCount, field.rangeBegin - 1u)) {
                continue;
            } else {
                Visit(reach, table, assign, field.target, target);
            }
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
            if (f.object != reach.kind[at] || f.kind == RK_EMBED)
                continue;
            ReplayObjectFieldKey(key, table, true, reach.kind[at], reach.object[at], f);
            ReplayFormatObjectField(f, reach.object[at], table, true, value);
            if (known.Changed(key.Data(), value.Data())) {
                out.Add("  ");
                out.AddChar(tag);
                out.AddChar(' ');
                out.Add(key.Data());
                out.AddChar(' ');
                out.Add(value.Data());
                out.AddChar('\n');
            }
        }
    }
    key.Free();
    value.Free();
}
