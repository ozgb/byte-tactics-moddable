#include <vector>

struct Elem_00434020 {
    unsigned short a;
    unsigned short b;
};

struct Elem_00434360 {
    std::vector<Elem_00434020> v;
};

typedef std::vector<Elem_00434360> W1_00433130;
typedef std::vector<W1_00433130> W2_00433130;

#include "../util/tdf.h"
#include "los_tables.h"

void __stdcall BuildDataPath(char* out, const char* dir, const char* name, const char* ext);

// FUNCTION: 0x433130
void LosTables::LoadLosTables()
{
    TdfFile tdf;
    char path[256];
    BuildDataPath(path, "gamedata", "los", "TDF");
    if ((&tdf)->LoadFile(path) != 0) {
        if ((&tdf)->SelectRecord("TABLEINFO") != 0) {
            short numtables = (short)tdf.current->GetFieldInt("numtables", 0);
            {
                W2_00433130& tables = *(W2_00433130*)this;
                W1_00433130 temp;
                unsigned n = (unsigned)numtables;
                if (tables.size() < n)
                    tables.insert(tables.end(), n - tables.size(), temp);
                else if (n < tables.size())
                    tables.erase(tables.begin() + n, tables.end());
            }
            for (short i = 0; i < numtables; i++)
                LoadLosTable(&tdf, i);
        }
        (&tdf)->Unload();
    }
}
