#ifndef GAME_CURSOR_STATE_H
#define GAME_CURSOR_STATE_H

#include "../../include/mouse_event.h"
#include "cursor_point.h"

#pragma pack(push, 1)
struct GameCursorState {
    MouseEvent rect;
    int flag;
    CursorPoint center;
    CursorPoint savedScroll;
};
#pragma pack(pop)

#if defined(__clang__) || !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(GameCursorState) == 44, "GameCursorState");
#endif

#endif
