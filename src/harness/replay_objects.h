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
    enum { KINDS = 16, HASH = 16384 };
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
