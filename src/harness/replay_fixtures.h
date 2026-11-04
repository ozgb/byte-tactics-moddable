#ifndef HARNESS_REPLAY_FIXTURES_H
#define HARNESS_REPLAY_FIXTURES_H

#include "../../interfaces/ref.h"
#include "../graphics/handle.h"
#include "../map/frame_table.h"

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

struct FixtureHolder {
    int id;
    Handle anim;
    Handle shadow;
};

#endif
