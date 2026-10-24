void __stdcall BuildDataPath(char* out, const char* dir, const char* name, const char* ext);
#include "../files/files_api.h"
void __stdcall FatalError(char* path);
#include "palette.h"
void __cdecl GameFreeThunk(void* p);
unsigned int* __stdcall BuildAlphaTable(void* palette);
unsigned int* __stdcall BuildShadeTable(void* palette);
unsigned int* __stdcall BuildLightTable(void* palette);
void __stdcall BuildGrayTable(unsigned int palette);
void __stdcall BuildBlueTable(unsigned int palette);

// FUNCTION: 0x42e140
unsigned int* __stdcall LoadAlphaTable(void* palette)
{
    char path[256];
    BuildDataPath(path, "palettes", "PALETTE", "ALP");
    if (HAPI_FileLengthByName(path)) {
        unsigned int* table = (unsigned int*)HAPI_LoadFile(path, 0);
        if (table == 0) {
            FatalError(path);
        }
        SetAlphaTable(table);
        GameFreeThunk(table);
        return table;
    }
    unsigned int* table = BuildAlphaTable(palette);
    WriteBufferToFile(path, table, 0x10000);
    return table;
}

// FUNCTION: 0x42e1d0
unsigned int* __stdcall LoadShadeTable(void* palette)
{
    char path[256];
    BuildDataPath(path, "palettes", "PALETTE", "SHD");
    if (HAPI_FileLengthByName(path)) {
        unsigned int* table = (unsigned int*)HAPI_LoadFile(path, 0);
        if (table == 0) {
            FatalError(path);
        }
        SetShadeTable(table);
        GameFreeThunk(table);
        return table;
    }
    unsigned int* table = BuildShadeTable(palette);
    WriteBufferToFile(path, table, 0x2000);
    return table;
}

// FUNCTION: 0x42e260
unsigned int* __stdcall LoadLightTable(void* palette)
{
    char path[256];
    BuildDataPath(path, "palettes", "PALETTE", "LHT");
    if (HAPI_FileLengthByName(path)) {
        unsigned int* table = (unsigned int*)HAPI_LoadFile(path, 0);
        if (table == 0) {
            FatalError(path);
        }
        SetLightTable(table);
        GameFreeThunk(table);
        return table;
    }
    unsigned int* table = BuildLightTable(palette);
    WriteBufferToFile(path, table, 0x2000);
    return table;
}

// FUNCTION: 0x42e2f0
void __stdcall MakeGrayTable(unsigned int palette)
{
    BuildGrayTable(palette);
}

// FUNCTION: 0x42e300
void __stdcall MakeBlueTable(unsigned int palette)
{
    BuildBlueTable(palette);
}
