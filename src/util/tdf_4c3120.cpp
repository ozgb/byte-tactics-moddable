#include <vector>
#include "../state/block.h"
#include <string.h>

#include "string_ref.h"

#include "../graphics/object_pool.h"
void* __cdecl operator new(unsigned int size);
void __cdecl operator delete(void* p);
#include "../debug/debug_lib.h"

#include <new>
#include "tdf.h"

// FUNCTION: 0x4c3120
void TdfFile::LoadBuffer(char* data, int size, int flag, char* path)
{
    delete root;
    root = 0;
    current = 0;
    flags = flag;
    char* text = (char*)GameAllocIgnoreTag("TDF file", size + 1);
    memcpy(text, data, size);
    text[size] = 0;
    this->StripComments(text);
    TdfRecord* node = (TdfRecord*)SimAlloc(0x29);
    root = node ? new (node) TdfRecord("root", text, 0, path) : 0;
    GameFreeThunk((int*)text);
}
