#ifndef GRID_H
#define GRID_H

#include "../../interfaces/ref.h"

struct Grid {
    Ref<unsigned char> cells;
    unsigned int width;
    unsigned int height;
    int count;
    Grid() { width = 0; height = 0; count = 0; cells = 0; }
};

#endif
