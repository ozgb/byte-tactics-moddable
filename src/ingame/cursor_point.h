#ifndef CURSOR_POINT_H
#define CURSOR_POINT_H

#pragma pack(push, 1)
struct CursorPoint {
    int x;
    int y;
};
#pragma pack(pop)

#if defined(__clang__) || !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(CursorPoint) == 8, "CursorPoint");
#endif

#endif
