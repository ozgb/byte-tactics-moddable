#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <memory>
#include <vector>

#include "replay_schema.h"
#include "../../interfaces/host.h"
#include "../../interfaces/ref.h"
#include "../util/engine_services.h"
#include "../../interfaces/seams.h"
#include "../../interfaces/rand_stream.h"
#include "../game/los_helpers.h"
#include "../state/game_rand.h"
#include "../state/block.h"
#include "../util/vec3.h"
#include "../units/unit.h"
#include "replay_text.h"
#include "replay_types.h"
#include "replay_values.h"
#include "replay_objects.h"
#include "replay_callbacks.h"
#include "replay_services.h"
#include "replay.h"

namespace {

enum HandleKind { OBJ_FILE, OBJ_RECORD, OBJ_BANK, OBJ_CALLBACK };

static_assert(OBJ_CALLBACK == REPLAY_HANDLE_CALLBACK, "the callback handles are numbered as the object fields number them");

struct Recording {
    Recording() : on(true), windowed(false), first(0), last(0) {}
    bool on;
    bool windowed;
    unsigned first;
    unsigned last;
    ReplayText out;
    ReplayKnown known;
    ReplayObjectTable table;
    ReplayCacheStart cache;
    ReplayCacheStart callCache;
};

struct ReplayRoots {
    constexpr ReplayRoots() : count(0), kind(), object() {}
    template <class T> void Add(unsigned short objectKind, T* p)
    {
        if (count < REPLAY_MAX_ROOTS && p != nullptr) {
            kind[count] = objectKind;
            object[count++] = (void*)p;
        }
    }
    unsigned count;
    unsigned short kind[REPLAY_MAX_ROOTS];
    void* object[REPLAY_MAX_ROOTS];
};

Recording* s_rec[SUB_COUNT];
bool s_open;
bool s_recording;
unsigned s_slot;
int s_kind = -1;
unsigned s_sub;
int s_tick;
ReplayText s_call;
ReplayText s_tail;
ReplayText s_field;
ReplaySnapshots s_snapshots;
ReplayText s_nested;
bool s_nestedSlot;
ReplayReach s_reach;
ReplayRoots s_roots;

ReplayObjectTable& Table()
{
    return s_rec[s_sub]->table;
}

void AddStream(ReplayText& t, int stream)
{
    ReplayValue(t, (RandStream)stream);
}

void AddHandle(ReplayText& t, unsigned kind, const void* p)
{
    static const char* const names[] = { "file", "rec", "bank" };
    unsigned ordinal = ReplayHandles().Of(kind, p);
    if (ordinal == 0) {
        t.Add("null");
        return;
    }
    t.Add(kind >= OBJ_CALLBACK ? kReplayCallbacks[kind - OBJ_CALLBACK].handle : names[kind]);
    t.AddChar('#');
    t.AddUnsigned(ordinal);
}

void AddFloatBits(ReplayText& t, float value)
{
    ReplayValue(t, value);
}

void AddDoubleBits(ReplayText& t, double value)
{
    ReplayValue(t, value);
}

void AddVec3(ReplayText& t, const Vec3* v)
{
    ReplayValue(t, *v);
}

#pragma push_macro("g_randomSeed")
#undef g_randomSeed

unsigned RandomSeed()
{
    return g_simGlobals->g_randomSeed;
}

#pragma pop_macro("g_randomSeed")

struct RegionState {
    void* ptr;
    unsigned size;
};

RegionState s_regions[SUB_COUNT];
ReplayRegionView s_regionView;

void RegionBind(unsigned sub, void* ptr, unsigned size)
{
    s_regions[sub].ptr = ptr;
    s_regions[sub].size = size;
    SimBlockTrack(&s_regions[sub].ptr);
}

const unsigned char* RegionViewOf()
{
    const RegionState& r = s_regions[s_sub];
    s_regionView.size = r.size;
    s_regionView.data = r.ptr == nullptr ? 0u : (unsigned)((const unsigned char*)r.ptr - (const unsigned char*)SimBlockBase());
    return (const unsigned char*)&s_regionView;
}

bool RecordsAlways(unsigned slot);

const unsigned char* BlockBase(const ReplayField& f)
{
    if (f.base == RB_REGION)
        return RegionViewOf();
    if (f.base == RB_GAME)
        return (const unsigned char*)g_game;
    return f.base == RB_SIM ? (const unsigned char*)g_simGlobals : nullptr;
}

bool RngReadable(const ReplayField& f)
{
    return SimBlockBase() != nullptr && (f.offset != 2 || g_simGlobals != nullptr);
}

unsigned RngValue(const ReplayField& f)
{
    return f.offset == 2 ? RandomSeed() : GameHoldrand((RandStream)f.offset);
}

unsigned s_rngAt[3];

void MarkRng()
{
    for (unsigned i = 0; i < kReplayFieldCount; i++) {
        const ReplayField& f = kReplayFields[i];
        if (f.base == RB_RNG && RngReadable(f))
            s_rngAt[f.offset] = RngValue(f);
    }
}

void ImportRng(ReplayText& t, int id)
{
    if (id == RI_RANDOM_INT || id == RI_GAME_RAND) {
        MarkRng();
        return;
    }
    for (unsigned i = 0; i < kReplayFieldCount; i++) {
        const ReplayField& f = kReplayFields[i];
        if (f.base != RB_RNG || !RngReadable(f) || RngValue(f) == s_rngAt[f.offset])
            continue;
        s_rngAt[f.offset] = RngValue(f);
        t.Add("\n  o ");
        t.Add(f.name);
        t.AddChar(' ');
        t.AddHex32(s_rngAt[f.offset]);
    }
}

void CaptureSet(const unsigned char* set, unsigned count, char tag)
{
    Recording& r = *s_rec[s_sub];
    for (unsigned i = 0; i < count; i++) {
        const ReplayField& f = kReplayFields[set[i]];
        if (set[i] == RF_GAME_TICKS)
            continue;
        if (f.base == RB_RNG) {
            if (SimBlockBase() == nullptr || (f.offset == 2 && g_simGlobals == nullptr))
                continue;
            s_field.Clear();
            s_field.AddHex32(f.offset == 2 ? RandomSeed() : GameHoldrand((RandStream)f.offset));
            if (r.known.Changed(f.name, s_field.Data())) {
                s_call.Add("  ");
                s_call.AddChar(tag);
                s_call.AddChar(' ');
                s_call.Add(f.name);
                s_call.AddChar(' ');
                s_call.Add(s_field.Data());
                s_call.AddChar('\n');
            }
            continue;
        }
        if (BlockBase(f) == nullptr)
            continue;
        ReplayEmitField(s_call, r.known, r.table, tag, f.name, ReplayFieldView(f), BlockBase(f), s_field);
    }
}

void CaptureObjectSet(const ReplaySlotSets& sets, bool entry)
{
    Recording& r = *s_rec[s_sub];
    ReplayWalk(s_reach, r.table, true, sets, s_roots.kind, s_roots.object, s_roots.count);
    if (entry)
        ReplayCaptureObjects(s_call, r.known, r.table, s_reach, sets.objReads, sets.objReadCount, 'r');
    else
        ReplayCaptureObjects(s_call, r.known, r.table, s_reach, sets.objWrites, sets.objWriteCount, 'w');
}

const ReplaySlotSets& SetsOfKind(unsigned slot, int kind)
{
    for (unsigned i = 0; i < kReplayKindSetCount; i++) {
        if (kReplayKindSets[i].slot == slot && kReplayKindSets[i].kind == kind)
            return *kReplayKindSets[i].sets;
    }
    return kReplaySlots[slot];
}

const void** s_freed;
unsigned s_freedCount;
unsigned s_freedCapacity;

int FreedIndex(const void* p)
{
    for (unsigned i = 0; i < s_freedCount; i++) {
        if (s_freed[i] == p)
            return (int)i;
    }
    return -1;
}

bool Freed(const void* p)
{
    return FreedIndex(p) >= 0;
}

void MarkFreed(const void* p)
{
    if (Freed(p))
        return;
    if (s_freed == nullptr || s_freedCount == s_freedCapacity) {
        unsigned wanted = s_freedCapacity == 0 ? 64 : s_freedCapacity * 2;
        const void** grown = (const void**)realloc((void*)s_freed, wanted * sizeof(const void*));
        if (grown == nullptr) {
            abort();
            return;
        }
        s_freed = grown;
        s_freedCapacity = wanted;
    }
    s_freed[s_freedCount++] = p;
}

const void** s_importFreed;
unsigned s_importFreedCount;
unsigned s_importFreedCapacity;

bool ImportFreedHas(const void* p)
{
    for (unsigned i = 0; i < s_importFreedCount; i++) {
        if (s_importFreed[i] == p)
            return true;
    }
    return false;
}

void ImportFreed(const void* p)
{
    ReplaySetWalkSkip(ImportFreedHas);
    if (p == nullptr || ImportFreedHas(p))
        return;
    if (s_importFreedCount == s_importFreedCapacity) {
        unsigned wanted = s_importFreedCapacity == 0 ? 64 : s_importFreedCapacity * 2;
        const void** grown = (const void**)realloc((void*)s_importFreed, wanted * sizeof(const void*));
        if (grown == nullptr) {
            abort();
            return;
        }
        s_importFreed = grown;
        s_importFreedCapacity = wanted;
    }
    s_importFreed[s_importFreedCount++] = p;
}

void MarkAllocated(const void* p)
{
    int at = FreedIndex(p);
    if (at >= 0)
        s_freed[at] = s_freed[--s_freedCount];
    for (unsigned i = 0; i < s_importFreedCount; i++) {
        if (s_importFreed[i] == p) {
            s_importFreed[i] = s_importFreed[--s_importFreedCount];
            break;
        }
    }
}

void AddLiveRoots(ReplayRoots& to, const ReplayRoots& from)
{
    for (unsigned i = 0; i < from.count; i++) {
        if (!Freed(from.object[i]))
            to.Add(from.kind[i], from.object[i]);
    }
}

unsigned short s_importChanged[kReplayObjectFieldCount];
ReplayReach s_importReach;

enum { BEFORE_MAX = 1 << 18, BEFORE_HASH = 1 << 19, BEFORE_BYTES = 1 << 20 };

struct BeforeEntry {
    const unsigned char* object;
    unsigned short field;
    unsigned at;
};

BeforeEntry s_before[BEFORE_MAX];
int s_beforeSlots[BEFORE_HASH];
unsigned char s_beforeBytes[BEFORE_BYTES];
unsigned s_beforeCount;
unsigned s_beforeUsed;
bool s_beforeTaken;
ReplayReach s_beforeReach;
ReplayKnown s_beforeKnown;
struct CallBlock {
    const unsigned char* first;
    unsigned bytes;
};
CallBlock* s_callBlocks;
unsigned s_callBlockCount;
unsigned s_callBlockCapacity;

unsigned BeforeHash(const void* object, unsigned field)
{
    return (unsigned)(((size_t)object >> 2) * 2654435761u + field * 40503u) % BEFORE_HASH;
}

bool WrittenFieldSaved(const ReplayObjectField& f)
{
    return f.kind != RK_EMBED && f.kind != RK_EMBEDS && f.kind != RK_UNITID && f.kind != RK_SELF;
}

void ImportBefore(const ReplayRoots* extra)
{
    s_beforeTaken = false;
    if (!s_recording || !kReplaySubsystems[s_sub].recordsImportChanges)
        return;
    Recording& r = *s_rec[s_sub];
    const ReplaySlotSets& sets = SetsOfKind(s_slot, s_kind);
    if (sets.objWriteCount == 0)
        return;
    ReplayRoots roots;
    AddLiveRoots(roots, s_roots);
    if (extra != nullptr)
        AddLiveRoots(roots, *extra);
    ReplayWalk(s_beforeReach, r.table, false, sets, roots.kind, roots.object, roots.count);
    memset(s_beforeSlots, 0xff, sizeof s_beforeSlots);
    s_beforeCount = 0;
    s_beforeUsed = 0;
    for (unsigned at = 0; at < s_beforeReach.count; at++) {
        const unsigned char* object = (const unsigned char*)s_beforeReach.object[at];
        for (unsigned j = 0; j < sets.objWriteCount; j++) {
            const ReplayObjectField& f = kReplayObjectFields[sets.objWrites[j]];
            if (f.object != s_beforeReach.kind[at] || !WrittenFieldSaved(f))
                continue;
            if (s_beforeCount == BEFORE_MAX || s_beforeUsed + f.size > BEFORE_BYTES) {
                ReplayFail("the fields a slot writes, saved before an import, pass the room of the recorder");
                return;
            }
            BeforeEntry& e = s_before[s_beforeCount];
            e.object = object;
            e.field = sets.objWrites[j];
            e.at = s_beforeUsed;
            memcpy(s_beforeBytes + s_beforeUsed, object + f.offset, f.size);
            s_beforeUsed += f.size;
            unsigned slot = BeforeHash(object, e.field);
            while (s_beforeSlots[slot] >= 0)
                slot = (slot + 1) % BEFORE_HASH;
            s_beforeSlots[slot] = (int)s_beforeCount++;
        }
    }
    s_beforeTaken = true;
}

void CallAllocated(const void* p, unsigned bytes)
{
    if (s_callBlocks == nullptr || s_callBlockCount == s_callBlockCapacity) {
        unsigned wanted = s_callBlockCapacity == 0 ? 64 : s_callBlockCapacity * 2;
        CallBlock* grown = (CallBlock*)realloc(s_callBlocks, wanted * sizeof(CallBlock));
        if (grown == nullptr) {
            abort();
            return;
        }
        s_callBlocks = grown;
        s_callBlockCapacity = wanted;
    }
    s_callBlocks[s_callBlockCount].first = (const unsigned char*)p;
    s_callBlocks[s_callBlockCount].bytes = bytes;
    s_callBlockCount++;
}

void CallFreed(const void* p)
{
    for (unsigned i = 0; i < s_callBlockCount; i++) {
        if (s_callBlocks[i].first == p) {
            s_callBlocks[i] = s_callBlocks[--s_callBlockCount];
            return;
        }
    }
}

bool InCallBlock(const unsigned char* object)
{
    for (unsigned i = 0; i < s_callBlockCount; i++) {
        if (object >= s_callBlocks[i].first && object < s_callBlocks[i].first + s_callBlocks[i].bytes)
            return true;
    }
    return false;
}

const BeforeEntry* SavedBefore(const unsigned char* object, unsigned field)
{
    for (unsigned slot = BeforeHash(object, field); s_beforeSlots[slot] >= 0; slot = (slot + 1) % BEFORE_HASH) {
        const BeforeEntry& e = s_before[s_beforeSlots[slot]];
        if (e.object == object && e.field == field)
            return &e;
    }
    return nullptr;
}

bool KnownByPlainKey(const ReplayObjectField& f)
{
    return f.kind != RK_ARRAY && f.kind != RK_BLOB && f.kind != RK_VECTOR && f.kind != RK_REFS && f.kind != RK_CSTRINGS;
}

void AddBeforeLine(ReplayText& lines, Recording& r, const char* key, const ReplayObjectField& f, const unsigned char* bytes, ReplayText& value)
{
    static unsigned char image[0x10000];
    if (f.offset + f.size > sizeof image) {
        ReplayFail("a field the slot writes lies past the room the recorder keeps for its value before an import");
        return;
    }
    memcpy(image + f.offset, bytes, f.size);
    ReplayFormatObjectField(f, image, r.table, true, value);
    const char* known = r.known.Get(key);
    if (known == nullptr || strcmp(known, value.Data()) == 0)
        return;
    lines.Add("  b ");
    lines.Add(key);
    lines.AddChar(' ');
    lines.Add(value.Data());
    lines.AddChar('\n');
}

void KnowObjectLines(ReplayKnown& known, const ReplayText& lines)
{
    std::vector<char> line;
    for (const char* at = lines.Data(); at != nullptr && *at != '\0';) {
        const char* end = strchr(at, '\n');
        size_t length = end == nullptr ? strlen(at) : (size_t)(end - at);
        line.assign(at, at + length);
        line.push_back('\0');
        char* space = length > 4 && line[2] == 'o' ? strchr(&line[0] + 4, ' ') : nullptr;
        if (space != nullptr) {
            char* name = &line[0] + 4;
            *space = '\0';
            known.Set(name, space + 1);
        }
        at = end == nullptr ? at + length : end + 1;
    }
}

void ImportWrittenObjects(ReplayText& t, Recording& r, const ReplaySlotSets& sets)
{
    ReplayText key;
    ReplayText value;
    ReplayText lines;
    s_beforeKnown.Reset();
    for (unsigned at = 0; at < s_importReach.count; at++) {
        unsigned kind = s_importReach.kind[at];
        const unsigned char* object = (const unsigned char*)s_importReach.object[at];
        for (unsigned j = 0; j < sets.objWriteCount; j++) {
            const ReplayObjectField& f = kReplayObjectFields[sets.objWrites[j]];
            if (f.object != kind || !WrittenFieldSaved(f) || !ReplayInReach(s_importReach, at, sets.objWrites[j]) || !ReplayFieldLive(f, object))
                continue;
            const BeforeEntry* before = SavedBefore(object, sets.objWrites[j]);
            bool changed = before != nullptr && memcmp(s_beforeBytes + before->at, object + f.offset, f.size) != 0;
            ReplayObjectFieldKey(key, r.table, true, kind, object, f);
            if (changed && KnownByPlainKey(f))
                AddBeforeLine(lines, r, key.Data(), f, s_beforeBytes + before->at, value);
            if (!changed) {
                bool stated = KnownByPlainKey(f) ? r.known.Get(key.Data()) != nullptr : before != nullptr;
                if (stated || InCallBlock(object))
                    continue;
            }
            ReplayEmitField(lines, s_beforeKnown, r.table, 'o', key.Data(), f, object, value);
        }
    }
    KnowObjectLines(r.known, lines);
    t.AddText(lines);
    lines.Free();
    key.Free();
    value.Free();
    s_beforeTaken = false;
}

void ImportObjects(ReplayText& t, const ReplayRoots* extra)
{
    if (!s_recording || !kReplaySubsystems[s_sub].recordsImportChanges)
        return;
    Recording& r = *s_rec[s_sub];
    const ReplaySlotSets& sets = SetsOfKind(s_slot, s_kind);
    unsigned n = 0;
    for (unsigned i = 0; i < sets.objReadCount; i++) {
        bool written = false;
        for (unsigned j = 0; j < sets.objWriteCount && !written; j++)
            written = sets.objWrites[j] == sets.objReads[i];
        if (!written)
            s_importChanged[n++] = sets.objReads[i];
    }
    if (n == 0 && !s_beforeTaken)
        return;
    ReplayRoots roots;
    AddLiveRoots(roots, s_roots);
    if (extra != nullptr)
        AddLiveRoots(roots, *extra);
    ReplayWalk(s_importReach, r.table, true, sets, roots.kind, roots.object, roots.count);
    if (n != 0)
        ReplayCaptureObjects(t, r.known, r.table, s_importReach, s_importChanged, n, 'o');
    if (s_beforeTaken)
        ImportWrittenObjects(t, r, sets);
}

void ImportServiceObjects(ReplayText& t, const ReplayImport& c)
{
    if (!s_recording || !kReplaySubsystems[s_sub].recordsImportChanges)
        return;
    if (c.id == RI_SIM_FREE) {
        MarkFreed(c.args[0].v.p);
        CallFreed(c.args[0].v.p);
        return;
    }
    if ((c.id == RI_SIM_ALLOC || c.id == RI_SIM_ALLOC_TAGGED || c.id == RI_SIM_CAPTURE_ALLOC) && c.result.kind == 1) {
        MarkAllocated(c.result.v.p);
        CallAllocated(c.result.v.p, (unsigned)c.args[c.id == RI_SIM_ALLOC_TAGGED ? 1 : 0].v.i);
    }
    ImportObjects(t, nullptr);
}

void SlotEnter(unsigned slot, unsigned sub, const ReplayRoots& roots, int kind)
{
    Recording& r = *s_rec[sub];
    s_slot = slot;
    s_kind = kind;
    s_sub = sub;
    s_tick = g_game == nullptr ? -1 : g_game->ticks;
    s_recording = !r.windowed || RecordsAlways(slot) || (s_tick >= 0 && (unsigned)s_tick >= r.first && (unsigned)s_tick <= r.last);
    if (!s_recording)
        return;
    if (strcmp(kReplaySubsystems[sub].name, "weapons") == 0) {
        ReplayCaptureDebrisCache(r.callCache);
        if (!r.cache.taken)
            r.cache = r.callCache;
    }
    ReplayHandles().Reset();
    s_snapshots.Reset();
    s_nested.Clear();
    s_call.Clear();
    s_tail.Clear();
    s_roots = roots;
    s_freedCount = 0;
    s_importFreedCount = 0;
    s_callBlockCount = 0;
    if (s_tick >= 0) {
        s_field.Clear();
        s_field.AddInt(s_tick);
        r.known.Set("Game.ticks", s_field.Data());
    }
    const ReplaySlotSets& sets = SetsOfKind(slot, kind);
    MarkRng();
    CaptureSet(sets.reads, sets.readCount, 'r');
    CaptureObjectSet(sets, true);
    ReplayOwner() = host_thread_id();
    ReplayCalls() = 1;
}

void SlotLeave()
{
    s_open = false;
    if (!s_recording)
        return;
    Recording& r = *s_rec[s_sub];
    ReplayCalls() = 0;
    const ReplaySlotSets& sets = SetsOfKind(s_slot, s_kind);
    CaptureSet(sets.writes, sets.writeCount, 'w');
    CaptureObjectSet(sets, false);
    if (strcmp(kReplaySubsystems[s_sub].name, "weapons") == 0) {
        ReplayCacheStart after;
        ReplayCaptureDebrisCache(after);
        ReplayAddCacheChanges(r.callCache, after, s_call);
    }
    r.out.Add("call ");
    if (s_tick >= 0)
        r.out.AddInt(s_tick);
    else
        r.out.AddChar('-');
    r.out.AddChar(' ');
    r.out.Add(sets.name);
    r.out.AddChar('\n');
    r.out.AddText(s_call);
    r.out.AddText(s_tail);
    s_snapshots.Emit(r.out);
    s_recording = false;
}

bool IsImportOf(unsigned owner, unsigned slot);
bool IsNestedSlot(unsigned slot);

class SlotScope {
public:
    SlotScope(unsigned slot, unsigned sub, const ReplayRoots& roots, bool recorded = true, int kind = -1) : role(0)
    {
        if (!s_open) {
            if (s_rec[sub] == nullptr || !recorded)
                return;
            role = 1;
            s_open = true;
            SlotEnter(slot, sub, roots, kind);
        } else if (sub != s_sub && s_rec[sub] != nullptr && recorded) {
            ReplayFail("a call of a recorded subsystem made inside a call of another recorded subsystem is not recorded: record each in its own run");
        } else if (s_recording && ReplayOwned() && IsImportOf(s_sub, slot)) {
            role = 2;
            ImportBefore(&roots);
            ReplayCalls() = 0;
        } else if (s_recording && ReplayNestedDepth() != 0 && !s_nestedSlot && ReplayOwner() == host_thread_id() && IsNestedSlot(slot)) {
            role = 3;
            s_nestedSlot = true;
            ReplayCalls() = 1;
        } else if (s_recording && ReplayCallbackDepth() != 0 && IsImportOf(s_sub, slot)) {
            ReplayFail("an import made inside a callback of an import is not recorded");
        }
    }
    ~SlotScope()
    {
        if (role == 1)
            SlotLeave();
        else if (role == 2)
            ReplayCalls() = 1;
        else if (role == 3) {
            s_nestedSlot = false;
            ReplayCalls() = 0;
        }
    }
    bool Open() const
    {
        return role == 1 && s_recording;
    }
    bool Import() const
    {
        return role == 2;
    }
    bool Nested() const
    {
        return role == 3;
    }

private:
    SlotScope(const SlotScope&);
    SlotScope& operator=(const SlotScope&);
    int role;
};

template <class T> void ArgValue(ReplayText& t, const char* name, T value, const char* tag = "a")
{
    t.Add("  ");
    t.Add(tag);
    t.AddChar(' ');
    t.Add(name);
    t.AddChar(' ');
    ReplayValue(t, value);
    t.AddChar('\n');
}

void ArgHandle(ReplayText& t, const char* name, unsigned kind, const void* p)
{
    t.Add("  a ");
    t.Add(name);
    t.AddChar(' ');
    AddHandle(t, kind, p);
    t.AddChar('\n');
}

void ArgObject(ReplayText& t, const char* name, unsigned kind, const void* p, const char* tag = "a")
{
    t.Add("  ");
    t.Add(tag);
    t.AddChar(' ');
    t.Add(name);
    t.AddChar(' ');
    ReplayAddObjectName(t, Table(), kind, p, true);
    t.AddChar('\n');
}

void ResultRoot(unsigned kind, const void* p)
{
    if (p == nullptr)
        return;
    unsigned actual = ReplayConcreteKind(kind, p);
    if (actual != kReplayObjectKindCount)
        s_roots.Add((unsigned short)actual, p);
}

void ArgStruct(ReplayText& t, const char* tag, const char* name, unsigned kind, const void* p)
{
    ReplayAddStruct(t, tag, name, kind, p, Table());
}

void ArgNullOut(ReplayText& t, const char* name, const void* p)
{
    if (p != nullptr)
        return;
    t.Add("  a ");
    t.Add(name);
    t.Add(" null\n");
}

void ArgEvent(ReplayText& t, const char* name, unsigned table, const void* p, unsigned size)
{
    ReplayAddEvent(t, name, table, p, size, Table());
}

void EventRoots(ReplayRoots& roots, unsigned table, const void* p, unsigned size)
{
    if (p == nullptr)
        return;
    unsigned char copy[REPLAY_EVENT_MAX];
    memset(copy, 0, sizeof copy);
    memcpy(copy, p, size < sizeof copy ? size : sizeof copy);
    unsigned object = ReplayEventObject(table, *(int*)copy);
    for (unsigned i = 0; i < kReplayObjectFieldCount; i++) {
        const ReplayObjectField& f = kReplayObjectFields[i];
        if (f.object == object && f.kind == RK_I32 && f.target == ROK_WEAPONDEF) {
            void* weapon = ReplayWeaponAt(*(int*)(copy + f.offset));
            if (weapon != nullptr)
                roots.Add(ROK_WEAPONDEF, weapon);
            continue;
        }
        if (f.object == object && f.kind == RK_I32 && f.target != 0) {
            void* element = ReplayTableElement(f.target, *(unsigned*)(copy + f.offset));
            if (element != nullptr)
                roots.Add(f.target, element);
            continue;
        }
        if (f.object != object || f.kind != RK_REF)
            continue;
        unsigned offset = *(unsigned*)(copy + f.offset);
        if (offset != 0)
            roots.Add(f.target, (char*)SimBlockBase() + offset);
    }
}

void ArgStructs(ReplayText& t, const char* name, unsigned kind, const void* p, int count)
{
    ReplayAddStructs(t, "a", name, kind, p, count, Table());
}

void ArgInOut(ReplayText& t, const char* name, unsigned kind, const void* p)
{
    t.Add("  a ");
    t.Add(name);
    t.AddChar(' ');
    ReplayAddInOut(t, kind, p, Table());
    t.AddChar('\n');
}

void ArgInOutExit(ReplayText& t, const char* name, unsigned kind, const void* p)
{
    if (p == nullptr)
        return;
    t.Add("  o ");
    t.Add(name);
    t.AddChar(' ');
    ReplayAddValue(t, kind, p, Table());
    t.AddChar('\n');
}

template <class T> void ArgInOutScalar(ReplayText& t, const char* name, const T* p)
{
    t.Add("  a ");
    t.Add(name);
    t.AddChar(' ');
    ReplayAddInOutScalar(t, p);
    t.AddChar('\n');
}

template <class T> void ArgInOutScalarExit(ReplayText& t, const char* name, const T* p)
{
    if (p == nullptr)
        return;
    t.Add("  o ");
    t.Add(name);
    t.AddChar(' ');
    ReplayAddInOutScalar(t, p);
    t.AddChar('\n');
}

void ArgValueStruct(ReplayText& t, const char* tag, const char* name, unsigned kind, const void* p)
{
    t.Add("  ");
    t.Add(tag);
    t.AddChar(' ');
    t.Add(name);
    t.AddChar(' ');
    ReplayAddValue(t, kind, p, Table());
    t.AddChar('\n');
}

void ArgArgv(ReplayText& t, const CommandArgs* args)
{
    t.Add("  a args.count ");
    t.AddInt(args->count);
    t.AddChar('\n');
    for (int i = 0; i < args->count && i < 0x14; i++) {
        t.Add("  a args[");
        t.AddInt(i);
        t.Add("] ");
        t.AddString(args->args[i]);
        t.AddChar('\n');
    }
}

void ArgWeapon(ReplayText& t, const char* name, const void* p)
{
    t.Add("  a ");
    t.Add(name);
    t.AddChar(' ');
    ReplayAddWeapon(t, p);
    t.AddChar('\n');
}

void ArgLoad(ReplayText& t, const char* name, unsigned load, const void* p)
{
    t.Add("  a ");
    t.Add(name);
    t.AddChar(' ');
    ReplayAddLoadName(t, Table(), load, p);
    t.AddChar('\n');
}

void ArgLoadResult(ReplayText& t, unsigned load, const void* p)
{
    t.Add("  o result ");
    ReplayAddLoadName(t, Table(), load, p);
    t.AddChar('\n');
}

void ImportLoad(ReplayText& t, unsigned load, const void* p)
{
    t.AddChar(' ');
    ReplayAddLoadName(t, Table(), load, p);
}

void ImportResultLoad(ReplayText& t, unsigned load, const void* p)
{
    t.Add(" = ");
    ReplayAddLoadName(t, Table(), load, p);
}

void ArgVec3(ReplayText& t, const char* name, const Vec3* v)
{
    ArgValue(t, name, v);
}

void ArgVec3Xz(ReplayText& t, const char* name, const Vec3* v)
{
    t.Add("  a ");
    t.Add(name);
    t.Add(" (");
    t.AddInt(v->x);
    t.AddChar(',');
    t.AddInt(v->z);
    t.Add(")\n");
}

void ArgPending(ReplayText& t, const char* name)
{
    t.Add("  a ");
    t.Add(name);
    t.Add(" ?\n");
}

template <class T> void ImportValue(ReplayText& t, T value)
{
    t.AddChar(' ');
    ReplayValue(t, value);
}

void ImportHandle(ReplayText& t, unsigned kind, const void* p)
{
    t.AddChar(' ');
    AddHandle(t, kind, p);
}

void ImportCallback(ReplayText& t, ReplayCallbackScope& scope, unsigned callback, void* object)
{
    unsigned kind = OBJ_CALLBACK + callback;
    unsigned ordinal = ReplayHandles().Of(kind, object);
    t.AddChar(' ');
    AddHandle(t, kind, object);
    scope.Start(callback, object, ordinal, Table(), ReplayThunkVtable(callback));
}

void ArgCallback(ReplayText& t, const char* name, ReplayCallbackScope& scope, unsigned callback, void* object)
{
    unsigned kind = OBJ_CALLBACK + callback;
    unsigned ordinal = ReplayHandles().Of(kind, object);
    t.Add("  a ");
    t.Add(name);
    t.AddChar(' ');
    AddHandle(t, kind, object);
    t.AddChar('\n');
    scope.Start(callback, object, ordinal, Table(), ReplayThunkVtable(callback), 2);
}

void ArgCallbackExit(ReplayText& t, const ReplayCallbackScope& scope)
{
    if (scope.Lines().Size() == 0)
        return;
    t.Add(scope.Lines().Data() + 1);
    t.AddChar('\n');
}

void ImportObject(ReplayText& t, unsigned kind, const void* p)
{
    t.AddChar(' ');
    ReplayAddObjectName(t, Table(), kind, p, true);
}

void ImportWeapon(ReplayText& t, const void* p)
{
    t.AddChar(' ');
    ReplayAddWeapon(t, p);
}

void ImportVec3(ReplayText& t, const char* label, const Vec3* v)
{
    t.AddChar(' ');
    t.Add(label);
    t.AddChar('=');
    ReplayValue(t, *v);
}

void ImportVec3Xz(ReplayText& t, const char* label, const Vec3* v)
{
    t.AddChar(' ');
    t.Add(label);
    t.Add("=(");
    t.AddInt(v->x);
    t.AddChar(',');
    t.AddInt(v->z);
    t.AddChar(')');
}

void ImportValueStruct(ReplayText& t, unsigned kind, const void* p)
{
    t.AddChar(' ');
    ReplayAddValue(t, kind, p, Table());
}

void ImportStructIn(ReplayText& t, unsigned kind, const void* p)
{
    if (p == nullptr)
        t.Add(" null");
    else
        ImportValueStruct(t, kind, p);
}

void ImportInOut(ReplayText& t, unsigned kind, const void* p)
{
    t.AddChar(' ');
    ReplayAddInOut(t, kind, p, Table());
}

void ImportEffect(ReplayText& t, unsigned table, const void* p, unsigned size)
{
    t.AddChar(' ');
    ReplayAddEffect(t, table, p, size, Table());
}

void ArgRegionOffset(ReplayText& t, const char* name, const void* region)
{
    ArgValue(t, name, (unsigned)((const unsigned char*)region - (const unsigned char*)SimBlockBase()));
}

void ArgRegionResult(ReplayText& t, const void* result)
{
    t.Add(result != nullptr && result == s_regions[s_sub].ptr ? "  o result region\n" : "  o result null\n");
}

void ImportOutInOut(ReplayText& t, const char* name, unsigned kind, const void* p)
{
    if (p == nullptr)
        return;
    t.Add("\n  o ");
    t.Add(name);
    t.AddChar(' ');
    ReplayAddValue(t, kind, p, Table());
}

template <class T> void ImportInOutScalar(ReplayText& t, const T* p)
{
    t.AddChar(' ');
    ReplayAddInOutScalar(t, p);
}

template <class T> void ImportOutInOutScalar(ReplayText& t, const char* name, const T* p)
{
    if (p == nullptr)
        return;
    t.Add("\n  o ");
    t.Add(name);
    t.AddChar(' ');
    ReplayAddInOutScalar(t, p);
}

template <class T> void ImportOutValues(ReplayText& t, const char* name, unsigned kind, const std::vector<T, std::allocator<T> >* values)
{
    t.Add("\n  o ");
    t.Add(name);
    t.AddChar(' ');
    ReplayAddValueList(t, kind, values->empty() ? nullptr : &(*values)[0], (unsigned)values->size(), Table());
}

void ImportResultValue(ReplayText& t, unsigned kind, const void* p)
{
    t.Add(" = ");
    ReplayAddValue(t, kind, p, Table());
}

void ImportPending(ReplayText& t)
{
    t.Add(" ?");
}

template <class T> void ImportResult(ReplayText& t, T value)
{
    t.Add(" = ");
    ReplayValue(t, value);
}

void ImportResultObject(ReplayText& t, unsigned kind, const void* p)
{
    t.Add(" = ");
    ReplayAddObjectName(t, Table(), kind, p, true);
}

void ArgWeaponResult(ReplayText& t, const void* p, unsigned count, ...)
{
    t.Add("  o result ");
    ReplayAddWeapon(t, p);
    t.AddChar('\n');
    if (p == nullptr)
        return;
    va_list names;
    va_start(names, count);
    for (unsigned n = 0; n < count; n++) {
        const char* name = va_arg(names, const char*);
        const ReplayObjectField* f = ReplayFindObjectField(ROK_WEAPONDEF, name);
        if (f == nullptr)
            continue;
        t.Add("  o ");
        ReplayAddWeapon(t, p);
        t.AddChar('.');
        t.Add(name);
        t.AddChar(' ');
        ReplayFormatObjectField(*f, p, Table(), true, s_field);
        t.Add(s_field.Data());
        t.AddChar('\n');
    }
    va_end(names);
}

void ImportResultWeapon(ReplayText& t, const void* p, unsigned count, ...)
{
    t.Add(" = ");
    ReplayAddWeapon(t, p);
    if (p == nullptr)
        return;
    va_list names;
    va_start(names, count);
    for (unsigned n = 0; n < count; n++) {
        const char* name = va_arg(names, const char*);
        const ReplayObjectField* f = ReplayFindObjectField(ROK_WEAPONDEF, name);
        if (f == nullptr)
            continue;
        t.Add("\n  o ");
        ReplayAddWeapon(t, p);
        t.AddChar('.');
        t.Add(name);
        t.AddChar(' ');
        ReplayFormatObjectField(*f, p, Table(), true, s_field);
        t.Add(s_field.Data());
    }
    va_end(names);
}

void ImportBytes(ReplayText& t, const void* p, int length)
{
    t.AddChar(' ');
    ReplayBytes(t, p, length);
}

void ImportOutBytes(ReplayText& t, const char* name, const void* p, int count, int length)
{
    if (count > length)
        count = length;
    if (count <= 0 || p == nullptr)
        return;
    t.Add("\n  o ");
    t.Add(name);
    t.AddChar(' ');
    ReplayBytes(t, p, count);
}

void ArgOutBytes(ReplayText& t, const char* name, const void* p, int count, int length)
{
    if (count > length)
        count = length;
    if (count <= 0 || p == nullptr)
        return;
    t.Add("  o ");
    t.Add(name);
    t.AddChar(' ');
    ReplayBytes(t, p, count);
    t.AddChar('\n');
}

template <class T> void ImportOutScalar(ReplayText& t, const char* name, const T* p)
{
    if (p == nullptr)
        return;
    t.Add("\n  o ");
    t.Add(name);
    t.AddChar(' ');
    ReplayValue(t, *p);
}

void ImportOutVec3(ReplayText& t, const char* name, const Vec3* v)
{
    t.Add("\n  o ");
    t.Add(name);
    t.AddChar(' ');
    ReplayValue(t, *v);
}

void ImportOutUnits(ReplayText& t, const char* name, const std::vector<Unit*, std::allocator<Unit*> >* units, const unsigned short* limit, unsigned limitCount)
{
    t.Add("\n  o ");
    t.Add(name);
    t.Add(" [");
    unsigned n = (unsigned)units->size();
    for (unsigned i = 0; i < n; i++) {
        if (i != 0)
            t.AddChar(',');
        ReplayAddObjectName(t, Table(), ROK_UNIT, (*units)[i], true);
    }
    t.AddChar(']');
    if (n == 0)
        return;
    std::vector<unsigned short> kinds(n, (unsigned short)ROK_UNIT);
    std::vector<void*> objects(n);
    for (unsigned i = 0; i < n; i++)
        objects[i] = (*units)[i];
    ReplaySlotSets none = {};
    ReplayWalk(s_reach, Table(), true, none, &kinds[0], &objects[0], n, limit, limitCount);
    ReplayText lines;
    ReplayCaptureObjects(lines, s_rec[s_sub]->known, Table(), s_reach, limit, limitCount, 'o');
    if (lines.Size() != 0) {
        t.AddChar('\n');
        const char* data = lines.Data();
        for (unsigned i = 0; i + 1 < lines.Size(); i++)
            t.AddChar(data[i]);
    }
    lines.Free();
}

void ImportOutUnitBuffer(ReplayText& t, const char* slot, const char* name, const Ref<Unit>* units, int result, int capacity, const unsigned short* limit, unsigned limitCount)
{
    if (result > capacity) {
        char message[200];
        _snprintf(message, sizeof message, "%s returned %d units for a buffer of %d: a recording would lose units", slot, result, capacity);
        message[sizeof message - 1] = '\0';
        ReplayFail(message);
        return;
    }
    std::vector<Unit*, std::allocator<Unit*> > list;
    for (int i = 0; i < result; i++)
        list.push_back(units[i].get());
    ImportOutUnits(t, name, &list, limit, limitCount);
}

void ArgOutUnitBuffer(ReplayText& t, const char* slot, const char* name, const Ref<Unit>* units, int result, int capacity)
{
    (void)slot;
    int held = result < capacity ? result : capacity;
    t.Add("  o ");
    t.Add(name);
    t.Add(" [");
    for (int i = 0; i < held; i++) {
        if (i != 0)
            t.AddChar(',');
        ReplayAddObjectName(t, Table(), ROK_UNIT, units[i].get(), true);
    }
    t.Add("]\n");
}

void ImportResultObjectReach(ReplayText& t, unsigned kind, const void* p, unsigned count, ...)
{
    t.Add(" = ");
    ReplayAddObjectName(t, Table(), kind, p, true);
    if (p == nullptr)
        return;
    va_list names;
    va_start(names, count);
    for (unsigned n = 0; n < count; n++) {
        const char* name = va_arg(names, const char*);
        const ReplayObjectField* f = ReplayFindObjectField(kind, name);
        if (f == nullptr)
            continue;
        t.Add("\n  o ");
        ReplayAddObjectName(t, Table(), kind, p, true);
        t.AddChar('.');
        t.Add(name);
        t.AddChar(' ');
        ReplayFormatObjectField(*f, p, Table(), true, s_field);
        const char* close = f->kind == RK_BLOB && s_field.Data()[0] == '[' ? strchr(s_field.Data(), ']') : nullptr;
        if (close == nullptr) {
            t.Add(s_field.Data());
            continue;
        }
        for (const char* c = s_field.Data(); c <= close; c++)
            t.AddChar(*c);
        if (close[1] != '\0') {
            t.Add(" 0:");
            t.Add(close + 1);
        }
    }
    va_end(names);
}

void ImportObjectOuts(ReplayText& t, unsigned kind, const void* p, unsigned count, ...)
{
    if (p == nullptr)
        return;
    va_list names;
    va_start(names, count);
    for (unsigned n = 0; n < count; n++) {
        const char* name = va_arg(names, const char*);
        const ReplayObjectField* f = ReplayFindObjectField(kind, name);
        if (f == nullptr)
            continue;
        t.Add("\n  o ");
        ReplayAddObjectName(t, Table(), kind, p, true);
        t.AddChar('.');
        t.Add(name);
        t.AddChar(' ');
        ReplayFormatObjectField(*f, p, Table(), true, s_field);
        t.Add(s_field.Data());
    }
    va_end(names);
}

void ImportResultPending(ReplayText& t)
{
    t.Add(" = ?");
}

void ImportArgv(ReplayText& t, const CommandArgs* args)
{
    t.AddChar(' ');
    ReplayAddArgs(t, args);
}

void FinishNested(ReplayText& line)
{
    s_nested.AddText(line);
    line.Free();
}

void FinishImport(ReplayText& line, const ReplayRoots& roots)
{
    s_call.AddText(line);
    s_call.AddChar('\n');
    line.Free();
    ImportObjects(s_call, &roots);
}

#include "replay_hooks.inc"

}

namespace {

void AddSpace(ReplayText& t)
{
    t.AddChar(' ');
}

void AddSnapshot(ReplayText& t, const void* buffer, unsigned bytes, unsigned offset)
{
    t.Add(" = ");
    if (buffer == nullptr) {
        t.Add("null");
        return;
    }
    t.Add("snapshot#");
    t.AddUnsigned(s_snapshots.Add(buffer, bytes));
    if (offset != 0) {
        t.AddChar('+');
        t.AddUnsigned(offset);
    }
}

void AddAllocName(ReplayText& t, const void* p)
{
    if (p == nullptr) {
        t.Add("null");
        return;
    }
    if (Table().Find(ROK_ALLOC, p) != 0) {
        ReplayAddObjectName(t, Table(), ROK_ALLOC, p, false);
        return;
    }
    for (unsigned i = 0; i < kReplayAllocTagCount; i++) {
        if (Table().Find(kReplayAllocTags[i].kind, p) != 0) {
            ReplayAddObjectName(t, Table(), kReplayAllocTags[i].kind, p, false);
            return;
        }
    }
    t.Add("alloc#?");
}

void AddAlloc(ReplayText& t, const void* p, const char* tag)
{
    t.Add(" = ");
    if (p == nullptr) {
        t.Add("null");
        return;
    }
    if (s_recording && kReplaySubsystems[s_sub].forgetsAllocations)
        Table().Forget(p);
    unsigned kind = ROK_ALLOC;
    for (unsigned i = 0; tag != nullptr && i < kReplayAllocTagCount; i++) {
        if (strcmp(kReplayAllocTags[i].tag, tag) == 0)
            kind = kReplayAllocTags[i].kind;
    }
    ReplayAddObjectName(t, Table(), kind, p, true);
}

void AddResult(ReplayText& t, const ReplayArg& r)
{
    if (r.kind == 0) {
        t.Add(" = ");
        t.AddInt(r.v.i);
    }
}

}

#include "replay_services.inc"

static void RecordLoad(unsigned load, const char* name, const void* data)
{
    int size = g_seams.files->LengthByName(name);
    if (size > 0)
        ReplayLoadNote(load, name, data, (unsigned)size);
}

void ReplayImportBegin(ReplayImport&)
{
    ImportBefore(nullptr);
}

int ReplayConfigure(const char* value, char* error, int size)
{
    char subsystem[32];
    unsigned first = 0;
    unsigned last = 0;
    int fields = sscanf(value, "%31s %u %u", subsystem, &first, &last);
    if (fields != 1 && fields != 3) {
        _snprintf(error, size, ": needs <subsystem> or <subsystem> <first tick> <last tick>");
        return 0;
    }
    unsigned sub = 0;
    while (sub < kReplaySubsystemCount && strcmp(kReplaySubsystems[sub].name, subsystem) != 0)
        sub++;
    if (sub == kReplaySubsystemCount || kReplaySubsystems[sub].slotCount == 0) {
        _snprintf(error, size, ": %s is not a simulation subsystem", subsystem);
        return 0;
    }
    if (s_rec[sub] != nullptr) {
        _snprintf(error, size, ": %s is recorded by an earlier record line", subsystem);
        return 0;
    }
    if (fields == 3 && (first == 0 || last < first)) {
        _snprintf(error, size, ": needs a first tick of at least 1 and a last tick not before it");
        return 0;
    }
    ReplayEndHook() = ReplayImportEnd;
    ReplayLoadHook() = RecordLoad;
    ReplayBeginHook() = ReplayImportBegin;
    s_rec[sub] = new Recording();
    s_rec[sub]->windowed = fields == 3;
    s_rec[sub]->first = first;
    s_rec[sub]->last = last;
    return 1;
}

void ReplayInstall()
{
    bool on[SUB_COUNT];
    bool any = false;
    for (unsigned i = 0; i < SUB_COUNT; i++) {
        on[i] = s_rec[i] != nullptr;
        any = any || on[i];
    }
    if (any)
        HookSeams(on);
}

int ReplayWrite(char* error, int size)
{
    int ok = 1;
    if (ReplayFailure() != nullptr) {
        _snprintf(error, size, "%s", ReplayFailure());
        return 0;
    }
    for (unsigned i = 0; i < SUB_COUNT; i++) {
        if (s_rec[i] == nullptr)
            continue;
        char name[96];
        sprintf(name, "replay-%s.txt", kReplaySubsystems[i].name);
        FILE* file = fopen(name, "wb");
        if (file == nullptr) {
            ok = 0;
            continue;
        }
        char head[160];
        sprintf(head, "bt-replay %d\nsubsystem %s %d.%d%s\n", REPLAY_FORMAT, kReplaySubsystems[i].name, (int)kReplaySubsystems[i].major,
                (int)kReplaySubsystems[i].minor, kReplaySubsystems[i].complete ? "" : " incomplete");
        ReplayText loads;
        ReplayAddLoadLines(s_rec[i]->table, loads);
        ReplayAddCacheLines(s_rec[i]->cache, loads);
        bool written = fputs(head, file) >= 0 && fwrite(loads.Data(), 1, loads.Size(), file) == loads.Size()
            && fwrite(s_rec[i]->out.Data(), 1, s_rec[i]->out.Size(), file) == s_rec[i]->out.Size();
        loads.Free();
        if (fclose(file) != 0 || !written)
            ok = 0;
    }
    return ok;
}
