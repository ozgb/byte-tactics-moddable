#ifndef LAYER_H
#define LAYER_H

struct Gadget;

#pragma pack(push, 1)

struct Layer {
    Layer* next;
    Gadget* entries;
    void (__stdcall* handler)(void*);
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
    void (__stdcall* textHandler)(void*);
};

#pragma pack(pop)

#endif
