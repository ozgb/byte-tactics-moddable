#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <memory>
#include <vector>

#include "replay_schema.h"
#include "../../interfaces/ref.h"
#include "../../interfaces/services.h"
#include "../../interfaces/seams.h"
#include "../../interfaces/rand_stream.h"
#include "../game/los_helpers.h"
#include "../state/game_rand.h"
#include "../util/vec3.h"
#include "../units/unit.h"
#include "replay_text.h"
#include "replay_types.h"
#include "replay_values.h"
#include "replay_objects.h"
#include "replay_callbacks.h"
#include "replay.h"

namespace {

enum HandleKind { OBJ_FILE, OBJ_RECORD, OBJ_BANK, OBJ_CALLBACK };

struct Recording {
    Recording() : on(true), windowed(false), first(0), last(0) {}
    bool on;
    bool windowed;
    unsigned first;
    unsigned last;
    ReplayText out;
    ReplayKnown known;
    ReplayObjectTable table;
};

struct ReplayRoots {
    constexpr ReplayRoots() : count(0), kind(), object() {}
    template <class T> void Add(unsigned char objectKind, T* p)
    {
        if (count < REPLAY_MAX_ROOTS && p != nullptr) {
            kind[count] = objectKind;
            object[count++] = (void*)p;
        }
    }
    unsigned count;
    unsigned char kind[REPLAY_MAX_ROOTS];
    void* object[REPLAY_MAX_ROOTS];
};

Recording* s_rec[SUB_COUNT];
bool s_open;
bool s_recording;
unsigned s_slot;
unsigned s_sub;
int s_tick;
ReplayText s_call;
ReplayText s_tail;
ReplayText s_field;
ReplayOrdinals s_ordinals;
ReplaySnapshots s_snapshots;
ReplayText s_nested;
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
    unsigned ordinal = s_ordinals.Of(kind, p);
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

const unsigned char* BlockBase(const ReplayField& f)
{
    if (f.base == RB_GAME)
        return (const unsigned char*)g_game;
    return f.base == RB_SIM ? (const unsigned char*)g_simGlobals : nullptr;
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

void SlotEnter(unsigned slot, unsigned sub, const ReplayRoots& roots)
{
    Recording& r = *s_rec[sub];
    s_slot = slot;
    s_sub = sub;
    s_tick = g_game == nullptr ? -1 : g_game->ticks;
    s_recording = !r.windowed || (s_tick >= 0 && (unsigned)s_tick >= r.first && (unsigned)s_tick <= r.last);
    if (!s_recording)
        return;
    s_ordinals.Reset();
    s_snapshots.Reset();
    s_call.Clear();
    s_tail.Clear();
    s_roots = roots;
    if (s_tick >= 0) {
        s_field.Clear();
        s_field.AddInt(s_tick);
        r.known.Set("Game.ticks", s_field.Data());
    }
    const ReplaySlotSets& sets = kReplaySlots[slot];
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
    const ReplaySlotSets& sets = kReplaySlots[s_slot];
    CaptureSet(sets.writes, sets.writeCount, 'w');
    CaptureObjectSet(sets, false);
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

class SlotScope {
public:
    SlotScope(unsigned slot, unsigned sub, const ReplayRoots& roots) : role(0)
    {
        if (!s_open) {
            if (s_rec[sub] == nullptr)
                return;
            role = 1;
            s_open = true;
            SlotEnter(slot, sub, roots);
        } else if (s_recording && ReplayOwned() && IsImportOf(s_sub, slot)) {
            role = 2;
            ReplayCalls() = 0;
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
    }
    bool Open() const
    {
        return role == 1 && s_recording;
    }
    bool Import() const
    {
        return role == 2;
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

void ArgStruct(ReplayText& t, const char* tag, const char* name, unsigned kind, const void* p)
{
    ReplayAddStruct(t, tag, name, kind, p, Table());
}

void ArgStructs(ReplayText& t, const char* name, unsigned kind, const void* p, int count)
{
    ReplayAddStructs(t, "a", name, kind, p, count, Table());
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

void ArgVec3(ReplayText& t, const char* name, const Vec3* v)
{
    ArgValue(t, name, v);
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
    unsigned ordinal = s_ordinals.Of(kind, object);
    t.AddChar(' ');
    AddHandle(t, kind, object);
    scope.Start(callback, object, ordinal, Table(), ReplayThunkVtable(callback));
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
    if (count <= 0 || p == nullptr)
        return;
    t.Add("\n  o ");
    t.Add(name);
    t.AddChar(' ');
    ReplayBytes(t, p, count < length ? count : length);
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
    std::vector<unsigned char> kinds(n, (unsigned char)ROK_UNIT);
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
        t.Add(s_field.Data());
    }
    va_end(names);
}

void ImportResultPending(ReplayText& t)
{
    t.Add(" = ?");
}

void FinishImport(ReplayText& line)
{
    s_call.AddText(line);
    s_call.AddChar('\n');
    line.Free();
}

#include "replay_hooks.inc"

}

namespace {

void AddSpace(ReplayText& t)
{
    t.AddChar(' ');
}

void AddSnapshot(ReplayText& t, const void* buffer, unsigned bytes)
{
    t.Add(" = ");
    if (buffer == nullptr) {
        t.Add("null");
        return;
    }
    t.Add("snapshot#");
    t.AddUnsigned(s_snapshots.Add(buffer, bytes));
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
    if (sub == kReplaySubsystemCount) {
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
        bool written = fputs(head, file) >= 0 && fwrite(s_rec[i]->out.Data(), 1, s_rec[i]->out.Size(), file) == s_rec[i]->out.Size();
        if (fclose(file) != 0 || !written)
            ok = 0;
    }
    return ok;
}
