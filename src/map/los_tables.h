#ifndef LOS_TABLES_H
#define LOS_TABLES_H

#include "../state/block_vector.h"
#include "../../interfaces/services.h"
#include "los_lines.h"
struct StringValueElem;

class TdfFile;

struct DamageTableEntry {
    StringRef name;
    int value;

    DamageTableEntry(const StringRef& n, int v) : name(n), value(v) {}
    void* AssignPair(DamageTableEntry* param);
};

class LosTables : public BlockVector<LosTable> {
public:
    void FreeTables();
    void LoadLosTable(TdfFile* file, short table);
    LosTable* GetLosTable(int n);
    int GetLosTableCount();
    void LoadLosTables();
    void ResizeTables(short n);
};

void __stdcall AssignRange(StringValueElem* first, StringValueElem* last, StringValueElem* value);
StringValueElem* __stdcall AssignRangeBack(StringValueElem*, StringValueElem*, StringValueElem*);
void __stdcall DestroyPoint(int);

void __stdcall StoreDwordIfDst(int*, int*);

void __stdcall DestroyLine(LosLineData* p);

#endif
