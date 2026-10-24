#include <string.h>

#include "gadget.h"

struct Data_004a0880 {
    int next;
    Gadget* entries;
};

struct Object_004a0880 {
    char unknown_0[0x18];
    Data_004a0880* data;
    char unknown_1c[0x64 - 0x1c];
    int current;
    char unknown_68[0x74 - 0x68];
    int length;
};

// FUNCTION: 0x4a0880
void __stdcall SetGadgetText(Object_004a0880* obj, int index, char* text)
{
    strcpy(obj->data->entries[index].u.text, text);
    if (obj->current == index) {
        obj->length = strlen(text);
    }
}
