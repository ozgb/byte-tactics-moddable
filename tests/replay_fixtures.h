#ifndef TESTS_REPLAY_FIXTURES_H
#define TESTS_REPLAY_FIXTURES_H

#include "../interfaces/ref.h"
#include "../src/graphics/handle.h"
#include "../src/map/frame_table.h"

struct UnitDef;

struct FixtureView {
    unsigned short flags;
    union {
        struct {
            int count;
            int total;
        } plain;
        struct {
            float scale;
            FrameTable* table;
            Ref<UnitDef> def;
        } linked;
    };
};

struct FixtureSlave {
    unsigned char player;
    Ref<FixtureView> master;
    union {
        struct {
            int moved;
        } ground;
        struct {
            float speed;
        } air;
    };
    union {
        struct {
            int low;
        } narrow;
        struct {
            float high;
        } wide;
    };
};

struct FixtureCell {
    unsigned short unit;
    unsigned short unit2;
    unsigned char height;
    unsigned char high;
    unsigned char low;
    unsigned char metal;
    unsigned short feature;
    unsigned short spot;
    unsigned short flags;
};

struct FixtureLane {
    int level;
    unsigned short unit;
    unsigned short flags;
};

struct FixtureHolder {
    int id;
    Handle anim;
    Handle shadow;
};

class FixtureTimerBase {
public:
    virtual int Kind() { return 0; }
    unsigned short ticks;
};

class FixtureTimer : public FixtureTimerBase {
public:
    virtual int Kind() { return 1; }
    unsigned short period;
};

class FixtureAssault : public FixtureTimerBase {
public:
    virtual int Kind() { return 2; }
    int squad;
};

struct FixtureManager {
    int id;
    Ref<FixtureTimerBase> timers[4];
};

struct FixtureGroup {
    int level;
    short mode;
};

struct FixtureOwner {
    int id;
    Ref<FixtureGroup> groups;
};

struct FixtureUser {
    int tag;
    Ref<FixtureGroup> group;
};

struct FixtureCb {
    void** ppVtbl;
    int done;
};

struct FixtureSlot {
    short aim;
    FixtureCb cb;
    int reload;
};

struct FixtureRig {
    int id;
    FixtureSlot slots[3];
};

struct FixtureChannel {
    unsigned short pc;
    short state;
    int stack;
};

struct FixtureScript {
    int id;
    FixtureChannel channels[4];
};

struct FixtureOrder;

class FixtureAttach {
public:
    virtual int Kind() { return 0; }
    Ref<FixtureOrder> owner;
    Ref<FixtureAttach> next;
    int value;
};

struct FixtureOrder {
    int id;
    FixtureAttach link;
};

#endif
