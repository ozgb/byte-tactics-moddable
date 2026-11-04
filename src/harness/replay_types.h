#ifndef HARNESS_REPLAY_TYPES_H
#define HARNESS_REPLAY_TYPES_H

enum ReplayBase { RB_GAME, RB_SIM, RB_RNG };
enum ReplayKind { RK_I32, RK_U32, RK_U16, RK_F32, RK_POINT16, RK_STRING, RK_WEAPON, RK_I16, RK_REF, RK_FLAGS, RK_U8, RK_F64, RK_BYTES, RK_EMBED, RK_UNITID, RK_ARRAY, RK_CODE, RK_VECTOR, RK_PTR };
enum ReplayVectorMode { RV_OBJECT, RV_SCALAR, RV_VALUE };
enum { RT_ORDER = 0 };
enum ReplayScope { RS_STATE, RS_INDEX, RS_VALUE };

struct ReplayFactor {
    unsigned offset;
    unsigned char size;
    unsigned char shift;
};

struct ReplayLength {
    ReplayFactor first;
    ReplayFactor second;
};

struct ReplayArraySpec {
    unsigned char elem;
    unsigned char ref;
    unsigned capacity;
    ReplayLength length;
};

struct ReplayVectorSpec {
    unsigned char mode;
    unsigned char kind;
    unsigned size;
};

struct ReplayTableSpec {
    unsigned char sim;
    unsigned char ref;
    unsigned offset;
    ReplayLength count;
    unsigned alloc;
};

struct ReplayField {
    const char* name;
    unsigned offset;
    unsigned size;
    unsigned char base;
    unsigned char kind;
    unsigned short array;
    unsigned char target;
};

struct ReplayInsideSite {
    unsigned char kind;
    unsigned char container;
    unsigned offset;
    const char* member;
};

struct ReplayObjectKind {
    const char* name;
    unsigned size;
    unsigned char scope;
    unsigned char table;
    unsigned char insideFirst;
    unsigned char insideCount;
    unsigned char polyFirst;
    unsigned char polyCount;
    unsigned char delta;
};

struct ReplayCallbackType {
    const char* name;
    const char* handle;
    unsigned methodCount;
    const char* const* methods;
};

struct ReplayAllocTag {
    const char* tag;
    unsigned char kind;
    unsigned size;
};

struct ReplayCodeTable {
    const char* name;
    const void* const* entries;
    unsigned count;
};

struct ReplayCondition {
    unsigned short field;
    unsigned short via;
    unsigned char kind;
    unsigned mask;
    int value;
};

struct ReplayObjectField {
    const char* name;
    unsigned offset;
    unsigned size;
    unsigned char object;
    unsigned char kind;
    unsigned char target;
    unsigned short rangeEnd;
    unsigned short rangeBegin;
    ReplayCondition when;
    unsigned short reachFirst;
    unsigned short reachCount;
    unsigned short array;
};

struct ReplaySlotSets {
    const char* name;
    unsigned char subsystem;
    const unsigned char* reads;
    unsigned readCount;
    const unsigned char* writes;
    unsigned writeCount;
    const unsigned short* objReads;
    unsigned objReadCount;
    const unsigned short* objWrites;
    unsigned objWriteCount;
    const unsigned char* allOf;
    unsigned allOfCount;
};

struct ReplaySubsystem {
    const char* name;
    unsigned major;
    unsigned minor;
    unsigned char complete;
    unsigned firstSlot;
    unsigned slotCount;
};

#endif
