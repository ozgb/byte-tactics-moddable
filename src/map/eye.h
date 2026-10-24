#ifndef EYE_H
#define EYE_H

#include "../util/vec3.h"
#include "../../interfaces/ref.h"

struct ScreenPos {
    short x;
    short y;
};

struct Eye {
    Ref<char> player;
    Ref<ScreenPos> screen;
    short x;
    unsigned char flagA;
    char flagB;
    Ref<char> flagPtr;
    Vec3 pos;
    unsigned int expires;
    ScreenPos screenPos;
    Eye& operator=(const Eye& s) {
        player = s.player;
        screen = &screenPos;
        x = s.x;
        flagPtr = &flagB;
        pos = s.pos;
        flagA = s.flagA;
        expires = s.expires;
        screenPos = s.screenPos;
        flagB = s.flagB;
        return *this;
    }
};

#endif
