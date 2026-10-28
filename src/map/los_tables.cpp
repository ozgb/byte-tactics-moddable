#include "win_types.h"
#include <algorithm>
#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <vector>
#include "los_tables.h"

#include "../util/string_ref.h"

struct OwnedString {
    char* data;

    ~OwnedString() { ((StringRef*)this)->ReleaseRef(); }
};

struct StringValueElem {
    StringRef handle;
    int value;
};

#pragma pack(push, 1)
#include "../units/unit_def.h"
#pragma pack(pop)

typedef int (__stdcall* Compare)(const UnitDef&, const UnitDef&);

#include "los_lines.h"

typedef std::vector<LosLineData> LineList;

#include "../util/tdf.h"
#include "../game/data_files.h"

typedef std::vector<LosStep> LosTableView;

struct IntValueElem {
    int value;
};

#include "../orders/approach_areas.h"
#include "../state/sim_globals.h"

extern void __cdecl operator delete(void*);

typedef std::vector<OwnedString> OwnedStringRow;
typedef std::vector<OwnedStringRow> OwnedStringRowList;
typedef OwnedStringRowList& (OwnedStringRowList::*OwnedStringRowListAssignFn)(const OwnedStringRowList&);

OwnedStringRowListAssignFn g_assign_00432ba0 = &OwnedStringRowList::operator=;

// FUNCTION: 0x432ba0 ??1?$vector@UOwnedString@@V?$allocator@UOwnedString@@@std@@@std@@QAE@XZ

typedef std::vector<OwnedString> OwnedStringVectorSize;
typedef OwnedStringVectorSize::size_type (OwnedStringVectorSize::*OwnedStringVectorSizeFn)() const;

// FUNCTION: 0x432be0 ?size@?$vector@UOwnedString@@V?$allocator@UOwnedString@@@std@@@std@@QBEIXZ
OwnedStringVectorSizeFn g_size_00432be0 = &OwnedStringVectorSize::size;

// FUNCTION: 0x432c80
void __stdcall AssignRange(StringValueElem* first, StringValueElem* last, StringValueElem* value)
{
    for (; first != last; ++first) {
        first->handle.Assign(&value->handle);
        first->value = value->value;
    }
}

// FUNCTION: 0x432cb0
StringValueElem* __stdcall AssignRangeBack(StringValueElem* param_1, StringValueElem* param_2, StringValueElem* param_3)
{
    while (param_1 != param_2) {
        --param_2;
        --param_3;
        param_3->handle.Assign(&param_2->handle);
        param_3->value = param_2->value;
    }
    return param_3;
}

typedef std::allocator<DamageTableEntry> DamageTableEntryAllocator;
typedef void (DamageTableEntryAllocator::*DamageTableEntryConstructFn)(DamageTableEntry*, const DamageTableEntry&);

// FUNCTION: 0x432cf0 ?construct@?$allocator@UDamageTableEntry@@@std@@QAEXPAUDamageTableEntry@@ABU3@@Z
DamageTableEntryConstructFn g_construct_00432cf0 = &DamageTableEntryAllocator::construct;

// FUNCTION: 0x432d20
void* DamageTableEntry::AssignPair(DamageTableEntry* param)
{
    name.Assign(&param->name);
    value = param->value;
    return this;
}

#pragma auto_inline(off)
// FUNCTION: 0x4c93b0
StringRef* StringRef::Assign(const StringRef* param_1)
{
    *(int*)(param_1->ptr - 4) += 1;
    *(int*)(ptr - 4) -= 1;
    if (*(int*)(ptr - 4) == 0) {
        free(ptr - 4);
    }
    ptr = param_1->ptr;
    return this;
}
#pragma auto_inline(on)

static __inline UnitDef* Copy_backward(UnitDef* F, UnitDef* L, UnitDef* X)
{
    while (F != L)
        *--X = *--L;
    return X;
}

static void __inline Unguarded_insert(UnitDef* L, UnitDef V, Compare P)
{
    for (UnitDef* M = L; P(V, *--M); L = M)
        *L = *M;
    *L = V;
}

// FUNCTION: 0x432fb0
void __stdcall InsertionSortUnitTypes(UnitDef* first, UnitDef* last, Compare comp, void* tag)
{
    if (first != last)
        for (UnitDef* M = first; ++M != last; ) {
            UnitDef V = *M;
            if (!comp(V, *first))
                Unguarded_insert(M, V, comp);
            else {
                Copy_backward(first, M, M + 1);
                *first = V;
            }
        }
}

typedef std::vector<LineList> TableList;

// FUNCTION: 0x4330b0
void LosTables::FreeTables()
{
    ((TableList*)this)->~vector();
}

static inline LosTable* GetTable(LosTables* self, short n)
{
    n--;
    return (LosTable*)&(*(TableList*)self)[n];
}

// FUNCTION: 0x433380
void LosTables::LoadLosTable(TdfFile* file, short table)
{
    char name[32];
    sprintf(name, "TABLE%d", table + 1);
    file->ResetCurrentRecord();
    if (file->SelectRecord(name)) {
        LosTable* t = GetTable(this, table + 1);
        short numlines = (short)file->current->GetFieldInt("numlines", 0);
        t->SetNumLines(numlines);
        short n2 = numlines * 2;
        short n3 = numlines * 3;
        for (short i = 0; i < numlines; i++) {
            ((LosLine*)t->GetLine(i))->LoadLosLine(file, i, 0);
            ((LosLine*)t->GetLine(numlines + i))->LoadLosLine(file, i, 1);
            ((LosLine*)t->GetLine(n2 + i))->LoadLosLine(file, i, 2);
            ((LosLine*)t->GetLine(n3 + i))->LoadLosLine(file, i, 3);
        }
    }
}

// FUNCTION: 0x433520
int LosTables::GetLosTableCount()
{
    return ((TableList*)this)->size();
}

typedef std::vector<LosStep> LosLineSteps;
typedef std::vector<LosLineSteps> LosTableLines;
typedef void (std::allocator<LosStep>::*LosStepDestroyFn)(LosStep*);

LosStepDestroyFn g_destroy_00434020 = &std::allocator<LosStep>::destroy;

// FUNCTION: 0x433540
void LosTable::FreeLines()
{
    ((LosTableLines*)this)->~vector();
}

// FUNCTION: 0x4335c0
int LosTable::GetLosLineCount()
{
    return size();
}

// FUNCTION: 0x4335f0
void LosTable::ResizeLines(short n)
{
    LosLineData x;
    resize(n * 4, x);
}

// FUNCTION: 0x4335e0
LosLine* LosTable::GetLosLine(short param_1)
{
    return (LosLine*)&(*this)[param_1];
}

typedef std::vector<IntValueElem> IntValueElemRow;
typedef std::vector<IntValueElemRow> IntValueElemRowList;
typedef IntValueElemRowList& (IntValueElemRowList::*IntValueElemRowListAssignFn)(const IntValueElemRowList&);

IntValueElemRowListAssignFn g_assign_004336c0 = &IntValueElemRowList::operator=;

// FUNCTION: 0x4336c0 ??1?$vector@UIntValueElem@@V?$allocator@UIntValueElem@@@std@@@std@@QAE@XZ


// FUNCTION: 0x4336f0
void LosLine::LoadLosLine(TdfFile* obj, short line, short mode)
{
    char name[32];
    char buf[0x200];

    sprintf(name, "line%d", line + 1);
    if (obj->current->GetFieldString(buf, name, 0x200, DAT_005119b8) != 0) {
        char* tok = strtok(buf, ", ");
        if (tok == 0)
            return;
        short n = atoi(tok);
        LosStep x;
        resize(n, x);
        short i;
        switch (mode) {
        case 0:
            for (i = 0; i < n; i++) {
                (*this)[i].a = atoi(tok = strtok(0, ", "));
                (*this)[i].b = -atoi(tok = strtok(0, ", "));
            }
            break;
        case 1:
            for (i = 0; i < n; i++) {
                (*this)[i].b = atoi(tok = strtok(0, ", "));
                (*this)[i].a = atoi(tok = strtok(0, ", "));
            }
            break;
        case 2:
            for (i = 0; i < n; i++) {
                (*this)[i].a = -atoi(tok = strtok(0, ", "));
                (*this)[i].b = atoi(tok = strtok(0, ", "));
            }
            break;
        case 3:
            for (i = 0; i < n; i++) {
                (*this)[i].b = -atoi(tok = strtok(0, ", "));
                (*this)[i].a = -atoi(tok = strtok(0, ", "));
            }
            break;
        }
    } else {
        LosStep x;
        resize(0, x);
    }
}

// FUNCTION: 0x4339c0
int LosLine::GetLosLineStepCount()
{
    return size();
}

// FUNCTION: 0x4339e0
void LosLine::GetLosLineStep(short index, unsigned short* out1, unsigned short* out2)
{
    int idx = index;
    unsigned char* base = (unsigned char*)*(void**)((char*)this + 4);
    unsigned char* ptr = base + idx * 4;
    unsigned short val1 = *(unsigned short*)ptr;
    *out1 = val1;
    unsigned short val2 = *(unsigned short*)(ptr + 2);
    *out2 = val2;
}

bool operator==(const LosStep&, const LosStep&);
bool operator<(const LosStep&, const LosStep&);

template class std::vector<LosStep>;

// FUNCTION: 0x433a10 ??0?$vector@ULosStep@@V?$allocator@ULosStep@@@std@@@std@@QAE@ABV?$allocator@ULosStep@@@1@@Z

typedef std::vector<LosLineData> LosLineDataDtorVector;
typedef LosLineDataDtorVector& (LosLineDataDtorVector::*LosLineDataDtorAssignFn)(const LosLineDataDtorVector&);

LosLineDataDtorAssignFn g_assign_00433a30 = &LosLineDataDtorVector::operator=;

// FUNCTION: 0x433a30 ??1?$vector@ULosStep@@V?$allocator@ULosStep@@@std@@@std@@QAE@XZ

typedef std::vector<LosStep> LosStepVectorSize;
typedef LosStepVectorSize::size_type (LosStepVectorSize::*LosStepVectorSizeFn)() const;

// FUNCTION: 0x433a60 ?size@?$vector@ULosStep@@V?$allocator@ULosStep@@@std@@@std@@QBEIXZ
LosStepVectorSizeFn g_size_00433a60 = &LosStepVectorSize::size;

typedef std::vector<LosLineData> LosLineDataVectorSize;
typedef LosLineDataVectorSize::size_type (LosLineDataVectorSize::*LosLineDataVectorSizeFn)() const;

// FUNCTION: 0x433b00 ?size@?$vector@ULosLineData@@V?$allocator@ULosLineData@@@std@@@std@@QBEIXZ
LosLineDataVectorSizeFn g_size_00433b00 = &LosLineDataVectorSize::size;

typedef std::vector<LosStep> LosStepVectorInsert;
typedef void (LosStepVectorInsert::*LosStepVectorInsertFn)(
    LosStepVectorInsert::iterator, LosStepVectorInsert::size_type, const LosStep&);

// FUNCTION: 0x433b20 ?insert@?$vector@ULosStep@@V?$allocator@ULosStep@@@std@@@std@@QAEXPAULosStep@@IABU3@@Z
LosStepVectorInsertFn g_insert_00433b20 = &LosStepVectorInsert::insert;

typedef std::vector<LosStep> LosStepVectorErase;
typedef LosStepVectorErase::iterator (LosStepVectorErase::*LosStepVectorEraseFn)(
    LosStepVectorErase::iterator, LosStepVectorErase::iterator);

// FUNCTION: 0x433d50 ?erase@?$vector@ULosStep@@V?$allocator@ULosStep@@@std@@@std@@QAEPAULosStep@@PAU3@0@Z
LosStepVectorEraseFn g_erase_00433d50 = &LosStepVectorErase::erase;

typedef std::allocator<LosStep> LosStepAllocatorDeallocate;
typedef void (LosStepAllocatorDeallocate::*LosStepDeallocateFn)(void*, LosStepAllocatorDeallocate::size_type);

// FUNCTION: 0x433da0 ?deallocate@?$allocator@ULosStep@@@std@@QAEXPAXI@Z
LosStepDeallocateFn g_deallocate_00433da0 = &LosStepAllocatorDeallocate::deallocate;

typedef std::vector<LosLineData> LosLineDataVectorInsert;
typedef void (LosLineDataVectorInsert::*LosLineDataVectorInsertFn)(
    LosLineDataVectorInsert::iterator, LosLineDataVectorInsert::size_type, const LosLineData&);

// FUNCTION: 0x433db0 ?insert@?$vector@ULosLineData@@V?$allocator@ULosLineData@@@std@@@std@@QAEXPAULosLineData@@IABU3@@Z
LosLineDataVectorInsertFn g_insert_00433db0 = &LosLineDataVectorInsert::insert;

typedef std::vector<LosLineData> LosLineDataRow;
typedef std::vector<LosLineDataRow> LosLineDataRowList;
typedef void (LosLineDataRowList::*LosLineDataRowListInsertFn)(
    LosLineDataRowList::iterator, LosLineDataRowList::size_type, const LosLineDataRow&);

// FUNCTION: 0x4340f0 ?insert@?$vector@V?$vector@ULosLineData@@V?$allocator@ULosLineData@@@std@@@std@@V?$allocator@V?$vector@ULosLineData@@V?$allocator@ULosLineData@@@std@@@std@@@2@@std@@QAEXPAV?$vector@ULosLineData@@V?$allocator@ULosLineData@@@std@@@2@IABV32@@Z
LosLineDataRowListInsertFn g_insert_004340f0 = &LosLineDataRowList::insert;

typedef std::allocator<LosStep> LosStepAllocatorDestroy;
typedef void (LosStepAllocatorDestroy::*LosStepAllocatorDestroyFn)(LosStep*);

// FUNCTION: 0x4343f0 ?destroy@?$allocator@ULosStep@@@std@@QAEXPAULosStep@@@Z
LosStepAllocatorDestroyFn g_destroy_004343f0 = &LosStepAllocatorDestroy::destroy;

typedef std::allocator<LosLineData> LosLineDataAllocator;
typedef void (LosLineDataAllocator::*LosLineDataDestroyFn)(LosLineData*);

// FUNCTION: 0x434400 ?destroy@?$allocator@ULosLineData@@@std@@QAEXPAULosLineData@@@Z
LosLineDataDestroyFn g_destroy_00434400 = &LosLineDataAllocator::destroy;

// FUNCTION: 0x434430
void __stdcall DestroyPoint(int)
{
}

// FUNCTION: 0x434440
void __stdcall DestroyLine(LosLineData* p)
{
    std::_Destroy(p);
}

typedef std::vector<LosLineData> LosLineDataCopyVector;
typedef void (LosLineDataCopyVector::*LosLineDataCopyInsertFn)(
    LosLineDataCopyVector::iterator, LosLineDataCopyVector::size_type, const LosLineData&);

typedef std::vector<LosLineData> LosLineDataDeleteVector;
typedef LosLineDataDeleteVector& (LosLineDataDeleteVector::*LosLineDataDeleteAssignFn)(const LosLineDataDeleteVector&);

// FUNCTION: 0x434470 ??0LosLineData@@QAE@ABU0@@Z
LosLineDataCopyInsertFn g_insert_00434470 = &LosLineDataCopyVector::insert;

// FUNCTION: 0x4349f0 ??_GLosLineData@@QAEPAXI@Z
LosLineDataDeleteAssignFn g_assign_004349f0 = &LosLineDataDeleteVector::operator=;

// FUNCTION: 0x4345c0
void __stdcall StoreDwordIfDst(int* param_1, int* param_2)
{
    if (param_1 != 0) {
        *param_1 = *param_2;
    }
}

typedef std::vector<LosStep> LosStepVectorAssign;
typedef LosStepVectorAssign& (LosStepVectorAssign::*LosStepVectorAssignFn)(const LosStepVectorAssign&);

// FUNCTION: 0x4345e0 ??4?$vector@ULosStep@@V?$allocator@ULosStep@@@std@@@std@@QAEAAV01@ABV01@@Z
LosStepVectorAssignFn g_assign_004345e0 = &LosStepVectorAssign::operator=;

typedef std::vector<LosLineData> LosLineDataVectorAssign;
typedef LosLineDataVectorAssign& (LosLineDataVectorAssign::*LosLineDataVectorAssignFn)(const LosLineDataVectorAssign&);

// FUNCTION: 0x434770 ??4?$vector@ULosLineData@@V?$allocator@ULosLineData@@@std@@@std@@QAEAAV01@ABV01@@Z
LosLineDataVectorAssignFn g_assign_00434770 = &LosLineDataVectorAssign::operator=;

// FUNCTION: 0x433500
LosTableView* LosTables::GetLosTable(int n)
{
    return (LosTableView*)&(*(TableList*)this)[(short)(n - 1)];
}

// FUNCTION: 0x432d40
void __stdcall SortUnitTypes(UnitDef* first, UnitDef* last,
                            Compare comp, int unused)
{
    for (; std::_SORT_MAX < last - first; ) {
        UnitDef* _M = std::_Unguarded_partition(first, last,
            std::_Median(UnitDef(*first),
                UnitDef(*(first + (last - first) / 2)),
                UnitDef(*(last - 1)), comp), comp);
        if (last - _M <= _M - first)
            SortUnitTypes(_M, last, comp, 0), last = _M;
        else
            SortUnitTypes(first, _M, comp, 0), first = _M;
    }
}

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
                TableList& tables = *(TableList*)this;
                LineList temp;
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

// FUNCTION: 0x433270
void LosTables::ResizeTables(short n)
{
    LineList x;
    ((TableList*)this)->resize(n, x);
}

typedef TableList& (TableList::*TableListAssignFn)(const TableList&);

TableListAssignFn g_assign_00433a80 = &TableList::operator=;

// FUNCTION: 0x433a80 ??1?$vector@ULosLineData@@V?$allocator@ULosLineData@@@std@@@std@@QAE@XZ

typedef LineList::iterator (LineList::*LineListEraseFn)(
    LineList::iterator, LineList::iterator);

// FUNCTION: 0x434020 ?erase@?$vector@ULosLineData@@V?$allocator@ULosLineData@@@std@@@std@@QAEPAULosLineData@@PAU3@0@Z
LineListEraseFn g_erase_00434020 = &LineList::erase;

typedef void (TableList::*TableListInsertFn)(TableList::iterator, TableList::size_type, const LineList&);

// FUNCTION: 0x4344e0 ??0?$vector@ULosLineData@@V?$allocator@ULosLineData@@@std@@@std@@QAE@ABV01@@Z
TableListInsertFn g_insert_004344e0 = &TableList::insert;

typedef TableList::iterator (TableList::*TableListEraseFn)(
    TableList::iterator, TableList::iterator);

// FUNCTION: 0x434360 ?erase@?$vector@V?$vector@ULosLineData@@V?$allocator@ULosLineData@@@std@@@std@@V?$allocator@V?$vector@ULosLineData@@V?$allocator@ULosLineData@@@std@@@std@@@2@@std@@QAEPAV?$vector@ULosLineData@@V?$allocator@ULosLineData@@@std@@@2@PAV32@0@Z
TableListEraseFn g_erase_00434360 = &TableList::erase;
