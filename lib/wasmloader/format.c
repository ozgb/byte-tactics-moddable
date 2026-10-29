#include "format.h"

#include <stdio.h>

void wl_vformat(char* out, size_t size, const char* format, va_list args)
{
    if (size == 0) {
        return;
    }
    _vsnprintf(out, size, format, args);
    out[size - 1] = 0;
}

void wl_format(char* out, size_t size, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    wl_vformat(out, size, format, args);
    va_end(args);
}
