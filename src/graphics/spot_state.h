#ifndef SPOT_STATE_H
#define SPOT_STATE_H

#include "model_types.h"

struct UnitDrawView;
struct GafFrame;

#pragma pack(push, 1)
struct SpotState {
    int count;
    int cacheDrawCount;
    int animDirty;
    union { UnitDrawView* owner; void* ownerPointer; };
    GafFrame* bitmap;
    int shadow;
    Vector3s pos;
    ModelPiece* root;
    ModelPiece pieces[1];
    char unknown_58[18];
    int pos_x;
    int pos_y;
    int pos_z;
};
#pragma pack(pop)

#endif
