#ifndef OBJECT_VIEW_H
#define OBJECT_VIEW_H

#include "../../interfaces/ref.h"
#include "../util/vec3.h"
#include "cob_script.h"

#pragma pack(push, 1)
struct Object {
    char unknown_0[0x6a];
    Vec3 pos;
    char unknown_76[0x9a - 0x76];
    Ref<CobScript> script;
};
#pragma pack(pop)

#endif
