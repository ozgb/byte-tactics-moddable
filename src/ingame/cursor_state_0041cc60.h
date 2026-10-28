#ifndef CURSOR_STATE_0041CC60_H
#define CURSOR_STATE_0041CC60_H

#include "arg_00419670.h"
#include "point_0041cd50.h"

#pragma pack(push, 1)
struct GameCursorState {
    ViewMouseEvent rect;
    int flag;
    CursorPoint center;
    CursorPoint savedScroll;
};
#pragma pack(pop)

#if defined(__clang__) || !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(GameCursorState) == 44, "CursorState_0041cc60");
#endif

#endif
