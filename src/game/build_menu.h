#ifndef BUILD_MENU_H
#define BUILD_MENU_H

#pragma pack(push, 1)
struct BuildMenuEntry {
    unsigned short typeId;
    unsigned char page;
    unsigned char slot;
    char name[0x21];
};

struct BuildMenuPage {
    int count;
    BuildMenuEntry entries[5];
};
#pragma pack(pop)

#endif
