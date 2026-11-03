#ifndef HARNESS_REPLAY_TYPES_H
#define HARNESS_REPLAY_TYPES_H

enum ReplayBase { RB_GAME, RB_SIM, RB_RNG };
enum ReplayKind { RK_I32, RK_U32, RK_U16, RK_F32, RK_POINT16, RK_STRING, RK_WEAPON, RK_I16, RK_REF, RK_FLAGS };
enum ReplayScope { RS_STATE, RS_INDEX, RS_VALUE };

struct ReplayField {
    const char* name;
    unsigned offset;
    unsigned size;
    unsigned char base;
    unsigned char kind;
};

struct ReplayObjectKind {
    const char* name;
    unsigned size;
    unsigned char scope;
};

struct ReplayObjectField {
    const char* name;
    unsigned offset;
    unsigned size;
    unsigned char object;
    unsigned char kind;
    unsigned char target;
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
