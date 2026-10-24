#include "win_types.h"
#include <string.h>

#pragma pack(push, 1)
struct Cell {
    char unknown_0[0x7];
    unsigned char metal;
    char unknown_8[0x4];

    union {
        struct {
            unsigned char low : 3;
            unsigned char feature : 5;
        };
        struct {
            unsigned char low4 : 3;
            unsigned char feature4 : 4;
        };
    };
};

#include "../game/game.h"
#pragma pack(pop)

extern Game* g_game;

#include "../files/files_api.h"

// FUNCTION: 0x484ce0
void __stdcall SaveMetalPlotmap(HapiBank* file)
{
    file->OpenAccount("Metal");
    int size = g_game->width * g_game->height;
    unsigned char* buf = new unsigned char[size];
    Cell* cells = g_game->cells;
    for (int i = 0; i < size; i++) {
        buf[i] = cells[i].metal;
    }
    file->OpenNamedBox("Plotmap");
    file->WriteBox(buf, size);
    delete[] buf;
}

// FUNCTION: 0x484df0
void __stdcall SavePlayerFeaturesPlotmap(HapiBank* file)
{
    file->OpenAccount("PlayerFeatures");
    Cell* cells = g_game->cells;
    int size = g_game->width * g_game->height / 2;
    unsigned char* buf = new unsigned char[size];
    if (buf) {
        for (int i = 0; i < size; i++) {
            buf[i] = (cells[i * 2].feature << 4) | (cells[i * 2 + 1].feature & 0xf);
        }
        file->OpenNamedBox("Plotmap");
        file->WriteBox(buf, size);
        delete[] buf;
    }
}

// FUNCTION: 0x484d60
void __stdcall LoadMetalPlotmap(HapiBank* file)
{
    if (file->OpenAccount("Metal") && file->OpenNamedBox("Plotmap")) {
        int size = g_game->width * g_game->height;
        if (file->GetBoxSize() == size) {
            unsigned char* buf = new unsigned char[size];
            Cell* cells = g_game->cells;
            if ((unsigned int)file->ReadBox(buf, size) >= size) {
                for (int i = 0; i < size; i++) {
                    cells[i].metal = buf[i];
                }
                delete[] buf;
            }
        }
    }
}

// FUNCTION: 0x484e80
void __stdcall LoadPlayerFeaturesPlotmap(HapiBank* file)
{
    if (file->OpenAccount("PlayerFeatures") && file->OpenNamedBox("Plotmap")) {
        int size = g_game->width * g_game->height / 2;
        if (file->GetBoxSize() == size) {
            unsigned char* buf = new unsigned char[size];
            if (buf) {
                Cell* cells = g_game->cells;
                if ((unsigned int)file->ReadBox(buf, size) >= size) {
                    Cell* c = cells;
                    for (int i = 0; i < size; i++, c += 2) {
                        c->feature4 = buf[i] >> 4;
                        c[1].feature4 = buf[i] & 0xf;
                    }
                    delete[] buf;
                }
            }
        }
    }
}

// FUNCTION: 0x484f50
void __stdcall SaveMappingData(HapiBank* file)
{
    file->OpenAccount("Mapping");
    unsigned int size = g_game->width * g_game->height * sizeof(short) / 4;
    file->OpenNamedBox("Data");
    file->WriteBox((void *)g_game->visibilityMask, size);
}

// FUNCTION: 0x484fa0
void __stdcall LoadMappingData(HapiBank* file)
{
    if (file->OpenAccount("Mapping") && file->OpenNamedBox("Data")) {
        unsigned int size = g_game->width * g_game->height * sizeof(short) / 4;
        if (file->GetBoxSize() == size)
            file->ReadBox((void *)g_game->visibilityMask, size);
    }
}
