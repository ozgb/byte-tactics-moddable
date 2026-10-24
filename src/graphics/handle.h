#ifndef HANDLE_H
#define HANDLE_H

struct GafEntry;

struct Handle {
    unsigned short index;
    unsigned short value;
    unsigned char kind;
    char unknown_5[3];
    GafEntry* src;
};

#endif
