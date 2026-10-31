#ifndef FRAME_TABLE_H
#define FRAME_TABLE_H

#include <stddef.h>

#pragma pack(push, 1)
struct FrameTable {
    unsigned short count;
    char unknown_2[0x28 - 2];
    void* entries;
};
#pragma pack(pop)

static_assert(sizeof(FrameTable) == 44, "");
static_assert(offsetof(FrameTable, count) == 0, "");
static_assert(offsetof(FrameTable, entries) == 40, "");

#endif
