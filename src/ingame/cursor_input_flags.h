#ifndef CURSOR_INPUT_FLAGS_H
#define CURSOR_INPUT_FLAGS_H

#include "cursor_input_bits.h"

#pragma pack(push, 1)
union CursorInputFlags {
    union { unsigned char value; CursorInputBits bits; };
};
#pragma pack(pop)

#if defined(__clang__) || !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(CursorInputFlags) == 1, "CursorInputFlags");
#endif

#endif
