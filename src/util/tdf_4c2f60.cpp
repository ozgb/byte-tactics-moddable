#include <vector>
#include <string.h>

#include "string_ref.h"

struct Elem_004c2f60 {
    StringRef a;
    StringRef b;
};

void __stdcall ReleasePair(char* p);

namespace std {
inline void _Destroy(Elem_004c2f60* p)
{
    ReleasePair((char*)p);
}
}

void __cdecl GameFreeThunk(int* param_1);
void* __cdecl operator new(unsigned int size);
void __cdecl operator delete(void* p);
void* __cdecl GameAllocIgnoreTag(char* name, int size);

#include <new>
#include "tdf.h"

#include "../files/files_api.h"

// FUNCTION: 0x4c2f60
int TdfFile::LoadFile(char* path)
{
    FileHandle* file = HAPI_OpenFileRead(path);
    if (!file)
        return 0;
    int size = HAPI_FileLength(file);
    int result = 0;
    if (size > 0) {
        char* buf = (char*)GameAllocIgnoreTag(path, size);
        HAPI_SeekFile(file, 0);
        int got = HAPI_readfromfile(file, buf, size);
        if (got >= 0) {
            int flag = HAPI_IsInArchive(file);
            result = flag;
            if (root)
                delete root;
            root = 0;
            current = 0;
            flags = result;
            char* text = (char*)GameAllocIgnoreTag("TDF file", size + 1);
            memcpy(text, buf, size);
            text[size] = 0;
            this->StripComments(text);
            TdfRecord* node = (TdfRecord*)operator new(0x29);
            root = node ? new (node) TdfRecord("root", text, 0, path) : 0;
            GameFreeThunk((int*)text);
            result = 1;
        }
        GameFreeThunk((int*)buf);
    }
    HAPI_CloseFile(file);
    return result;
}
