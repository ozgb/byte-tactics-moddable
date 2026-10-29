#ifndef LAYER_H
#define LAYER_H

struct Gadget;
struct Gui;

#pragma pack(push, 1)

struct Layer {
    Layer* next;
    Gadget* entries;
    void (__stdcall* handler)(Gui*);
    void* data;
    unsigned int flags;
    int dirty;
    int keyboardInput;
    void (__stdcall* cb1c)();
    int current;
    void* surface;
    char text[0xe];
    char lastKey;
    int clickMode;
    void (__stdcall* textHandler)(Gui*);
};

#pragma pack(pop)

#endif
