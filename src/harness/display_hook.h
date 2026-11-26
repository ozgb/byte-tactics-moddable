#ifndef DISPLAY_HOOK_H
#define DISPLAY_HOOK_H

#define DL_NONE 0
#define DL_SPRITE 1
#define DL_SPRITE_OPAQUE 2
#define DL_SPRITE_BLEND 3
#define DL_SPRITE_GRAY 4
#define DL_SPRITE_SHADOW 5
#define DL_SPRITE_LIT 6
#define DL_SPRITE_ERASE 7
#define DL_LENS 8
#define DL_QUAD 9
#define DL_BLIT 10
#define DL_COPY 11
#define DL_CLEAR 12
#define DL_CLEAR_FRAME 13
#define DL_LINE 14
#define DL_PIXEL 15
#define DL_FADE 16
#define DL_FILL 17
#define DL_OUTLINE 18
#define DL_OUTLINE_LIT 19
#define DL_GRAY 20
#define DL_DITHER 21
#define DL_CIRCLE 22
#define DL_DASHED 23
#define DL_POLYGON 24
#define DL_TEXT 25
#define DL_TERRAIN 26
#define DL_MODEL 27
#define DL_PROJECTILE 28
#define DL_DEBRIS 29
#define DL_SHARD 30
#define DL_NO_ANGLES 100
#define DL_ALLOC_SURFACE 101
#define DL_ALLOC_FRAME 102
#define DL_GAF 103
#define DL_3DO 104
#define DL_FONT 105
#define DL_FEATURE_TYPE 106
#define DL_FRAME_END 107
#define DL_FREE 108
#define DL_WRITTEN 109

struct DlScope {
    DlScope(int kind, ...);
    ~DlScope();
};

void __cdecl DlNote(int kind, ...);

#endif
