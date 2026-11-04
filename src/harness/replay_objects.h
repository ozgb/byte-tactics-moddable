#ifndef HARNESS_REPLAY_OBJECTS_H
#define HARNESS_REPLAY_OBJECTS_H

#include "replay_text.h"
#include "replay_types.h"

enum { REPLAY_MAX_OBJECTS = 8192, REPLAY_MAX_ROOTS = 16, REPLAY_REACH_HASH = 16384 };

class ReplayObjectTable {
public:
    ReplayObjectTable() { Reset(); }
    void Reset();
    unsigned Find(unsigned kind, const void* p) const;
    unsigned Name(unsigned kind, const void* p);
    void* Pointer(unsigned kind, unsigned ordinal) const;
    void Bind(unsigned kind, unsigned ordinal, void* p);

private:
    ReplayObjectTable(const ReplayObjectTable&);
    ReplayObjectTable& operator=(const ReplayObjectTable&);
    enum { KINDS = 48, HASH = 16384 };
    unsigned count[KINDS];
    void* objects[KINDS][REPLAY_MAX_OBJECTS];
    short slots[KINDS][HASH];
};

struct ReplayReach {
    unsigned count;
    unsigned char kind[REPLAY_MAX_OBJECTS];
    void* object[REPLAY_MAX_OBJECTS];
    const unsigned short* limit[REPLAY_MAX_OBJECTS];
    unsigned short limitCount[REPLAY_MAX_OBJECTS];
    short slots[REPLAY_REACH_HASH];
};

typedef void* (*ReplayAllocFn)(unsigned size);

unsigned ReplayLengthOf(const ReplayLength& length, const void* object);
ReplayObjectField ReplayFieldView(const ReplayField& f);
void ReplayEmitField(ReplayText& out, ReplayKnown& known, ReplayObjectTable& table, char tag, const char* key, const ReplayObjectField& f, const void* object, ReplayText& scratch);
bool ReplayApplyArray(const ReplayObjectField& f, void* object, const char* key, const char* value, ReplayKnown& known, bool memory, ReplayAllocFn alloc);
void ReplayArrayDiff(const char* recorded, const char* replayed, ReplayText& out);
bool ReplayIsArrayText(const char* text);
void ReplayAddStruct(ReplayText& t, const char* tag, const char* name, unsigned kind, const void* p, ReplayObjectTable& table);
void ReplayAddStructs(ReplayText& t, const char* tag, const char* name, unsigned kind, const void* p, int count, ReplayObjectTable& table);
void ReplayAddValue(ReplayText& t, unsigned kind, const void* p, ReplayObjectTable& table);
bool ReplayParseValue(unsigned kind, const char* text, void* p, ReplayObjectTable& table, ReplayAllocFn alloc);

class ReplayCallbackScope {
public:
    ReplayCallbackScope();
    ~ReplayCallbackScope();
    void Start(unsigned callback, void* object, unsigned ordinal, ReplayObjectTable& table, const void* thunkVtable);
    void Stop();
    const ReplayText& Lines() const { return lines; }
    const void* Object() const { return object; }

private:
    ReplayCallbackScope(const ReplayCallbackScope&);
    ReplayCallbackScope& operator=(const ReplayCallbackScope&);
    friend class ReplayCallbackCall;
    bool active;
    unsigned callback;
    void* object;
    const void* original;
    unsigned ordinal;
    ReplayObjectTable* table;
    ReplayText lines;
};

class ReplayCallbackCall {
public:
    ReplayCallbackCall(const void* object, unsigned method);
    ~ReplayCallbackCall();
    bool Active() const { return scope != nullptr; }
    void Space() { text.AddChar(' '); }
    ReplayText& Text() { return text; }
    ReplayObjectTable& Table() { return *scope->table; }
    const void* Original(unsigned slot) const;

private:
    ReplayCallbackCall(const ReplayCallbackCall&);
    ReplayCallbackCall& operator=(const ReplayCallbackCall&);
    ReplayCallbackScope* scope;
    ReplayText text;
};

unsigned ReplayCallbackDepth();
const void* ReplayVptrOf(const void* object);
const void* ReplayVtable(unsigned kind);
const char* ReplayVtableName(unsigned kind);
void ReplayFail(const char* message);
const char* ReplayFailure();
void ReplayClearFailure();
unsigned ReplayConcreteKind(unsigned kind, const void* p);
bool ReplayKindFits(unsigned declared, unsigned actual);
bool ReplayInsideTable(unsigned kind, const void* p);
void ReplayAddWeapon(ReplayText& t, const void* p);
void* ReplayParseWeapon(const char* value);
void ReplayAddObjectName(ReplayText& t, ReplayObjectTable& table, unsigned kind, const void* p, bool assign);
bool ReplayFormatObjectField(const ReplayObjectField& f, const void* object, ReplayObjectTable& table, bool assign, ReplayText& out);
bool ReplayRestoreObjectField(const ReplayObjectField& f, void* object, const char* value, ReplayObjectTable& table, ReplayAllocFn alloc);
const ReplayObjectField* ReplayFindObjectField(unsigned kind, const char* name);
int ReplayFindObjectKind(const char* name, unsigned length);
void* ReplayResolveObject(const char* text, unsigned* kind, ReplayObjectTable& table, ReplayAllocFn alloc);
void ReplayWalk(ReplayReach& reach, ReplayObjectTable& table, bool assign, const ReplaySlotSets& sets, const unsigned char* rootKinds, void* const* roots, unsigned rootCount,
                const unsigned short* rootLimit = nullptr, unsigned rootLimitCount = 0);
bool ReplayInReach(const ReplayReach& reach, unsigned at, unsigned fieldIndex);
void ReplayCaptureObjects(ReplayText& out, ReplayKnown& known, ReplayObjectTable& table, const ReplayReach& reach, const unsigned short* set, unsigned count, char tag);
void ReplayObjectFieldKey(ReplayText& key, ReplayObjectTable& table, bool assign, unsigned kind, const void* object, const ReplayObjectField& f);
bool ReplayParseKey(const char* key, unsigned* kind, unsigned* ordinal, const char** field);

#endif
