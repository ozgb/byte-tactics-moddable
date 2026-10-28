#ifndef DISPLAY_FLAGS_VIEW_H
#define DISPLAY_FLAGS_VIEW_H

#pragma pack(push, 1)
struct DisplayFlagsView {
    char unknown_0[0xf0];
    unsigned short bit0 : 1;
    unsigned short fullscreen : 1;
    unsigned short rest : 14;
};
#pragma pack(pop)

#endif
