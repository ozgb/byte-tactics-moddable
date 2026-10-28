#ifndef BYTE_MAP_H
#define BYTE_MAP_H

#include "../../interfaces/ref.h"

struct ByteMapSize {
    unsigned int width;
    unsigned int height;

    int Contains(unsigned int tx, unsigned int ty) { return tx < width && ty < height; }
};

struct ByteMap {
    Ref<unsigned char> data;
    ByteMapSize size;

    unsigned char Get(int x, int y) { return data[size.width * y + x]; }
};

#endif
