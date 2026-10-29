#ifndef WASMLOADER_FORMAT_H
#define WASMLOADER_FORMAT_H

#include <stdarg.h>
#include <stddef.h>

void wl_vformat(char* out, size_t size, const char* format, va_list args);
void wl_format(char* out, size_t size, const char* format, ...);

#endif
