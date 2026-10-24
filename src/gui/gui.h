#ifndef GUI_H
#define GUI_H

#pragma pack(push, 1)

struct Layer;

struct Gui {
    int font;
    void* gaf;
    void* values[3];
    void* language;
    Layer* layer;
    int cursorSharedFrame;
    int cursorDefaultFrame;
    int cursorHoverFrame;
    void* src_28;
    void* src_2c;
    unsigned short cursorIndex;
    unsigned short cursorValue;
    unsigned char cursorKind;
    char unknown_35[3];
    void* cursorSrc;
    int pointX;
    int pointY;
    unsigned int pointKeyFlags;
    int pointTick;
    int pointMessage;
    int pointDoubleClick;
    int mouseKeyFlags;
    int clickMode;
    unsigned int cursorFlags;
    int hotGadgetIndex;
    int focus;
    int hoverGadgetIndex;
    int prevHoverGadget;
    int clearQuickKeys;
    int cursor;
    int knobDragging;
    char unknown_7c[0x96 - 0x7c];
    int time;
    int animTimer;
    int inputEnabled;
    int dirty;
    char unknown_a6[0xb2 - 0xa6];
    unsigned int palette[0x100];
    char unknown_4b2[0x8b2 - 0x4b2];
    unsigned char colours[0x100];
    char unknown_9b2[0x9b6 - 0x9b2];
    char str_9b6[0x100];
    char str_ab6[0x100];
    char str_bb6[0x110];
    int pathsReady;
    int changed;
    int clickStatusCache;
    void* fallback;
    char cachedBgName[0x20];
};

#pragma pack(pop)

#endif
