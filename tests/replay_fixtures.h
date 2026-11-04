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

#endif
