#ifndef WASMLOADER_JSON_H
#define WASMLOADER_JSON_H

#include <stddef.h>

enum {
    WL_JSON_NULL,
    WL_JSON_BOOL,
    WL_JSON_NUMBER,
    WL_JSON_STRING,
    WL_JSON_ARRAY,
    WL_JSON_OBJECT
};

typedef struct wl_json {
    int kind;
    long number;
    char* text;
    char* key;
    struct wl_json* first;
    struct wl_json* next;
} wl_json;

wl_json* wl_json_parse(const char* text, size_t size, const char* name, char* error, size_t error_size);
void wl_json_free(wl_json* node);
const wl_json* wl_json_get(const wl_json* object, const char* key);
size_t wl_json_count(const wl_json* node);

#endif
