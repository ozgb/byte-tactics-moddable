#ifndef HARNESS_REPLAY_OBJECTS_H
#define HARNESS_REPLAY_OBJECTS_H

#include "replay_text.h"
#include "replay_types.h"

enum { REPLAY_UNIT_POOL = 5001, REPLAY_MAX_OBJECTS = 8192, REPLAY_MAX_ROOTS = 16, REPLAY_REACH_HASH = 16384 };

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
    short slots[REPLAY_REACH_HASH];
};

typedef void* (*ReplayAllocFn)(unsigned size);

bool ReplayInsideTable(unsigned kind, const void* p);
void ReplayAddWeapon(ReplayText& t, const void* p);
void* ReplayParseWeapon(const char* value);
void ReplayAddObjectName(ReplayText& t, ReplayObjectTable& table, unsigned kind, const void* p, bool assign);
bool ReplayFormatObjectField(const ReplayObjectField& f, const void* object, ReplayObjectTable& table, bool assign, ReplayText& out);
bool ReplayRestoreObjectField(const ReplayObjectField& f, void* object, const char* value, ReplayObjectTable& table, ReplayAllocFn alloc);
const ReplayObjectField* ReplayFindObjectField(unsigned kind, const char* name);
int ReplayFindObjectKind(const char* name, unsigned length);
void* ReplayResolveObject(const char* text, unsigned* kind, ReplayObjectTable& table, ReplayAllocFn alloc);
void ReplayWalk(ReplayReach& reach, ReplayObjectTable& table, bool assign, const ReplaySlotSets& sets, const unsigned char* rootKinds, void* const* roots, unsigned rootCount);
void ReplayCaptureObjects(ReplayText& out, ReplayKnown& known, ReplayObjectTable& table, const ReplayReach& reach, const unsigned short* set, unsigned count, char tag);
void ReplayObjectFieldKey(ReplayText& key, ReplayObjectTable& table, bool assign, unsigned kind, const void* object, const ReplayObjectField& f);
bool ReplayParseKey(const char* key, unsigned* kind, unsigned* ordinal, const char** field);

#endif
