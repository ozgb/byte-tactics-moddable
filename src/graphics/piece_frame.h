#ifndef PIECE_FRAME_H
#define PIECE_FRAME_H

#pragma pack(push, 1)
struct PieceFrameFlags {
    unsigned int textured : 1;
    unsigned int usePic : 1;
    unsigned int shaded : 1;
    unsigned int rest : 29;
};

struct PieceFrame {
    int fillColor;
    int count;
    int always0;
    unsigned short* indices;
    int ptr_10;
    int texKind;
    unsigned short* color;
    PieceFrameFlags flags;
};
#pragma pack(pop)

#if defined(__clang__) || !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(PieceFrameFlags) == 4, "PieceFrameFlags");
static_assert(sizeof(PieceFrame) == 32, "PieceFrame");
#endif

#endif
