#ifndef DIALOG_H
#define DIALOG_H

#include "layer.h"

struct BlinkWord;
struct FileRequester;
struct Font_004a50b0;
struct Gadget;

#pragma pack(push, 1)

struct Point_0049fc50 {
    int x;
    int y;
    int unknown_8[4];
};

struct Dialog {
    union {
        struct {
            int unknown_0;
            Gadget* entries;
            void (__stdcall* handler)(void*);
            void* data;
            int flags;
            Font_004a50b0* language;
        };
        struct {
            char path[0x13];
            short field_13;
            char unknown_15[0x18 - 0x15];
        };
    };
    Layer* holder;
    void (__stdcall* field_1c)(void);
    int field_20;
    void* surface;
    char text[14];
    char field_36;
    int field_37;
    char unknown_3b[1];
    Point_0049fc50 point;
    unsigned int flags_54;
    char unknown_58[8];
    int current;
    int focus;
    int index;
    int used;
    int field_70;
    int length;
    int field_78;
    Point_0049fc50 saved;
    short field_94;
    char unknown_96[12];
    int keyCommands;
    BlinkWord* words;
    int count;
    int active;
    char unknown_b2[0x8b2 - 0xb2];
    unsigned char colour;
    char unknown_8b3[0x9b6 - 0x8b3];
    char path_9b6[0x100];
    char path_ab6[0x100];
    char path_bb6[0x100];
    char unknown_cb6[16];
    int field_cc6;
    int changed;

    FileRequester* OpenFileRequester(Dialog* self, char* arg2, char* arg3, char* arg4);
};

#pragma pack(pop)

#endif
