#ifndef ANIM_FILE_ENTRY_H
#define ANIM_FILE_ENTRY_H

#pragma pack(push, 1)
struct AnimFileEntry {
    char name[0x40];
    void* gaf;
};
#pragma pack(pop)

#endif
