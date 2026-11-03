#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include "../../interfaces/ref.h"
#include "replay_schema.h"
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
    if (kind >= KINDS || count[kind] == REPLAY_MAX_OBJECTS)
        abort();
    Bind(kind, count[kind] + 1, (void*)p);
    return count[kind];
}

void* ReplayObjectTable::Pointer(unsigned kind, unsigned ordinal) const
{
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
    unsigned ordinal = assign ? table.Name(kind, p) : table.Find(kind, p);
    t.Add(kReplayObjectKinds[kind].name);
    if (ordinal == 0) {
        t.Add("#?");
        return;
    }
    t.AddChar('#');
    t.AddUnsigned(ordinal);
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

void* ReplayResolveObject(const char* text, unsigned* kindOut, ReplayObjectTable& table, ReplayAllocFn alloc)
{
    const char* hash = strchr(text, '#');
    if (hash == nullptr)
        return nullptr;
    int kind = ReplayFindObjectKind(text, (unsigned)(hash - text));
    if (kind < 0)
        return nullptr;
    if (kindOut != nullptr)
        *kindOut = (unsigned)kind;
    if (hash[1] == '?')
        return nullptr;
    unsigned ordinal = (unsigned)atoi(hash + 1);
    if (kReplayObjectKinds[kind].scope == RS_INDEX)
        return ordinal < 256 && g_game != nullptr ? &g_game->weapons[ordinal] : nullptr;
    void* p = table.Pointer((unsigned)kind, ordinal);
    if (p == nullptr && ordinal != 0 && alloc != nullptr) {
        p = alloc(kReplayObjectKinds[kind].size);
        memset(p, 0, kReplayObjectKinds[kind].size);
        table.Bind((unsigned)kind, ordinal, p);
    }
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
    const char* dot = strchr(key, '.');
    if (hash == nullptr || dot == nullptr || dot < hash)
        return false;
    int k = ReplayFindObjectKind(key, (unsigned)(hash - key));
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

bool InObjectSets(const ReplaySlotSets& sets, unsigned field)
{
    for (unsigned i = 0; i < sets.objReadCount; i++) {
        if (sets.objReads[i] == field)
            return true;
    }
    for (unsigned i = 0; i < sets.objWriteCount; i++) {
        if (sets.objWrites[i] == field)
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
        if (reach.object[reach.slots[slot]] == object)
            return;
    }
    if (reach.count == REPLAY_MAX_OBJECTS)
        abort();
    reach.slots[slot] = (short)reach.count;
    if (assign)
        table.Name(kind, object);
    reach.kind[reach.count] = kind;
    reach.object[reach.count] = object;
    reach.count++;
}

}

void ReplayWalk(ReplayReach& reach, ReplayObjectTable& table, bool assign, const ReplaySlotSets& sets, const unsigned char* rootKinds, void* const* roots, unsigned rootCount)
{
    reach.count = 0;
    memset(reach.slots, 0xff, sizeof reach.slots);
    for (unsigned i = 0; i < rootCount; i++)
        Visit(reach, table, assign, rootKinds[i], roots[i]);
    for (unsigned at = 0; at < reach.count; at++) {
        for (unsigned f = 0; f < kReplayObjectFieldCount; f++) {
            const ReplayObjectField& field = kReplayObjectFields[f];
            if (field.object != reach.kind[at] || field.kind != RK_REF || !InObjectSets(sets, f))
                continue;
            unsigned offset = *(const unsigned*)((const unsigned char*)reach.object[at] + field.offset);
            Visit(reach, table, assign, field.target, BlockAt(offset));
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
            if (f.object != reach.kind[at])
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
