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

struct Elem_00432be0 {
    char* data;

    ~Elem_00432be0() { ((StringRef*)this)->ReleaseRef(); }
};

struct Elem_432cb0 {
    StringRef handle;
    int value;
};

#pragma pack(push, 1)
#include "../units/unit_def.h"
#pragma pack(pop)

typedef int (__stdcall* Compare)(const UnitDef&, const UnitDef&);

struct Elem_00434020 {
    unsigned short a;
    unsigned short b;
};

struct Elem_00434360 {
    std::vector<Elem_00434020> v;
};

typedef std::vector<Elem_00434360> Column_00433270;

#include "../util/tdf.h"
#include "../game/data_files.h"

typedef std::vector<Elem_00434020> Vec_004336f0;

class LosLine : public Vec_004336f0 {
public:
    int GetLosLineStepCount();
    void GetLosLineStep(short index, unsigned short* out1, unsigned short* out2);
    void LoadLosLine(TdfFile* obj, short line, short mode);
};

class LosTable : public std::vector<Elem_00434360> {
public:
    Elem_00434360* GetLine(short i) { return &(*this)[i]; }
    void SetNumLines(short n) { resize(n * 4); }

    int GetLosLineCount();
    void ResizeLines(short n);
    LosLine* GetLosLine(short i);
    void FreeLines();
};


typedef std::vector<Elem_00434020> Inner_00433500;

struct Elem_004336c0 {
    int value;
};

#include "../orders/approach_areas.h"
#include "../state/sim_globals.h"

extern void __cdecl operator delete(void*);

typedef std::vector<Elem_00432be0> Inner_00432ba0;
typedef std::vector<Inner_00432ba0> Outer_00432ba0;
typedef Outer_00432ba0& (Outer_00432ba0::*AssignFn_00432ba0)(const Outer_00432ba0&);

AssignFn_00432ba0 g_assign_00432ba0 = &Outer_00432ba0::operator=;

// FUNCTION: 0x432ba0 ??1?$vector@UElem_00432be0@@V?$allocator@UElem_00432be0@@@std@@@std@@QAE@XZ

typedef std::vector<Elem_00432be0> Vec_00432be0;
typedef Vec_00432be0::size_type (Vec_00432be0::*SizeFn_00432be0)() const;

// FUNCTION: 0x432be0 ?size@?$vector@UElem_00432be0@@V?$allocator@UElem_00432be0@@@std@@@std@@QBEIXZ
SizeFn_00432be0 g_size_00432be0 = &Vec_00432be0::size;

// FUNCTION: 0x432c00
void* StringRef::FUN_00432c00(unsigned char param_1)
{
    ReleaseRef();
    if ((param_1 & 1) != 0) {
        operator delete(this);
    }
    return this;
}

// FUNCTION: 0x432c20
void* StringRef::FUN_00432c20(unsigned char param_1)
{
    ReleaseRef();
    if ((param_1 & 1) != 0) {
        operator delete(this);
    }
    return this;
}

// FUNCTION: 0x432c80
void __stdcall AssignRange(Elem_432cb0* first, Elem_432cb0* last, Elem_432cb0* value)
{
    for (; first != last; ++first) {
        first->handle.Assign(&value->handle);
        first->value = value->value;
    }
}

// FUNCTION: 0x432cb0
Elem_432cb0* __stdcall AssignRangeBack(Elem_432cb0* param_1, Elem_432cb0* param_2, Elem_432cb0* param_3)
{
    while (param_1 != param_2) {
        --param_2;
        --param_3;
        param_3->handle.Assign(&param_2->handle);
        param_3->value = param_2->value;
    }
    return param_3;
}

typedef std::allocator<Entry_00432cf0> Alloc_00432cf0;
typedef void (Alloc_00432cf0::*ConstructFn_00432cf0)(Entry_00432cf0*, const Entry_00432cf0&);

// FUNCTION: 0x432cf0 ?construct@?$allocator@UEntry_00432cf0@@@std@@QAEXPAUEntry_00432cf0@@ABU3@@Z
ConstructFn_00432cf0 g_construct_00432cf0 = &Alloc_00432cf0::construct;

// FUNCTION: 0x432d20
void* Entry_00432cf0::AssignPair(Entry_00432cf0* param)
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

typedef std::vector<Column_00433270> Outer_004330b0;

// FUNCTION: 0x4330b0
void LosTables::FreeTables()
{
    ((Outer_004330b0*)this)->~vector();
}

static inline LosTable* GetTable(LosTables* self, short n)
{
    n--;
    return (LosTable*)&(*(Outer_004330b0*)self)[n];
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
    return ((Outer_004330b0*)this)->size();
}

typedef std::vector<Elem_00434020> Inner_00434020;
typedef std::vector<Inner_00434020> Outer_00434020;
typedef void (std::allocator<Elem_00434020>::*DestroyFn_00434020)(Elem_00434020*);

DestroyFn_00434020 g_destroy_00434020 = &std::allocator<Elem_00434020>::destroy;

// FUNCTION: 0x433540
void LosTable::FreeLines()
{
    ((Outer_00434020*)this)->~vector();
}

// FUNCTION: 0x4335c0
int LosTable::GetLosLineCount()
{
    return size();
}

// FUNCTION: 0x4335f0
void LosTable::ResizeLines(short n)
{
    Elem_00434360 x;
    resize(n * 4, x);
}

// FUNCTION: 0x4335e0
LosLine* LosTable::GetLosLine(short param_1)
{
    return (LosLine*)&(*this)[param_1];
}

typedef std::vector<Elem_004336c0> Inner_004336c0;
typedef std::vector<Inner_004336c0> Outer_004336c0;
typedef Outer_004336c0& (Outer_004336c0::*AssignFn_004336c0)(const Outer_004336c0&);

AssignFn_004336c0 g_assign_004336c0 = &Outer_004336c0::operator=;

// FUNCTION: 0x4336c0 ??1?$vector@UElem_004336c0@@V?$allocator@UElem_004336c0@@@std@@@std@@QAE@XZ


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
        Elem_00434020 x;
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
        Elem_00434020 x;
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

bool operator==(const Elem_00434020&, const Elem_00434020&);
bool operator<(const Elem_00434020&, const Elem_00434020&);

template class std::vector<Elem_00434020>;

// FUNCTION: 0x433a10 ??0?$vector@UElem_00434020@@V?$allocator@UElem_00434020@@@std@@@std@@QAE@ABV?$allocator@UElem_00434020@@@1@@Z

typedef std::vector<Elem_00434360> Outer_00433a30;
typedef Outer_00433a30& (Outer_00433a30::*AssignFn_00433a30)(const Outer_00433a30&);

AssignFn_00433a30 g_assign_00433a30 = &Outer_00433a30::operator=;

// FUNCTION: 0x433a30 ??1?$vector@UElem_00434020@@V?$allocator@UElem_00434020@@@std@@@std@@QAE@XZ

typedef std::vector<Elem_00434020> Vec_00433a60;
typedef Vec_00433a60::size_type (Vec_00433a60::*SizeFn_00433a60)() const;

// FUNCTION: 0x433a60 ?size@?$vector@UElem_00434020@@V?$allocator@UElem_00434020@@@std@@@std@@QBEIXZ
SizeFn_00433a60 g_size_00433a60 = &Vec_00433a60::size;

typedef std::vector<Elem_00434360> Vec_00433b00;
typedef Vec_00433b00::size_type (Vec_00433b00::*SizeFn_00433b00)() const;

// FUNCTION: 0x433b00 ?size@?$vector@UElem_00434360@@V?$allocator@UElem_00434360@@@std@@@std@@QBEIXZ
SizeFn_00433b00 g_size_00433b00 = &Vec_00433b00::size;

typedef std::vector<Elem_00434020> Vec_00433b20;
typedef void (Vec_00433b20::*InsertFn_00433b20)(
    Vec_00433b20::iterator, Vec_00433b20::size_type, const Elem_00434020&);

// FUNCTION: 0x433b20 ?insert@?$vector@UElem_00434020@@V?$allocator@UElem_00434020@@@std@@@std@@QAEXPAUElem_00434020@@IABU3@@Z
InsertFn_00433b20 g_insert_00433b20 = &Vec_00433b20::insert;

typedef std::vector<Elem_00434020> Vec_00433d50;
typedef Vec_00433d50::iterator (Vec_00433d50::*EraseFn_00433d50)(
    Vec_00433d50::iterator, Vec_00433d50::iterator);

// FUNCTION: 0x433d50 ?erase@?$vector@UElem_00434020@@V?$allocator@UElem_00434020@@@std@@@std@@QAEPAUElem_00434020@@PAU3@0@Z
EraseFn_00433d50 g_erase_00433d50 = &Vec_00433d50::erase;

typedef std::allocator<Elem_00434020> Alloc_00433da0;
typedef void (Alloc_00433da0::*DeallocateFn_00433da0)(void*, Alloc_00433da0::size_type);

// FUNCTION: 0x433da0 ?deallocate@?$allocator@UElem_00434020@@@std@@QAEXPAXI@Z
DeallocateFn_00433da0 g_deallocate_00433da0 = &Alloc_00433da0::deallocate;

typedef std::vector<Elem_00434360> Vec_00433db0;
typedef void (Vec_00433db0::*InsertFn_00433db0)(
    Vec_00433db0::iterator, Vec_00433db0::size_type, const Elem_00434360&);

// FUNCTION: 0x433db0 ?insert@?$vector@UElem_00434360@@V?$allocator@UElem_00434360@@@std@@@std@@QAEXPAUElem_00434360@@IABU3@@Z
InsertFn_00433db0 g_insert_00433db0 = &Vec_00433db0::insert;

typedef std::vector<Elem_00434360> Inner_004340f0;
typedef std::vector<Inner_004340f0> Outer_004340f0;
typedef void (Outer_004340f0::*InsertFn_004340f0)(
    Outer_004340f0::iterator, Outer_004340f0::size_type, const Inner_004340f0&);

// FUNCTION: 0x4340f0 ?insert@?$vector@V?$vector@UElem_00434360@@V?$allocator@UElem_00434360@@@std@@@std@@V?$allocator@V?$vector@UElem_00434360@@V?$allocator@UElem_00434360@@@std@@@std@@@2@@std@@QAEXPAV?$vector@UElem_00434360@@V?$allocator@UElem_00434360@@@std@@@2@IABV32@@Z
InsertFn_004340f0 g_insert_004340f0 = &Outer_004340f0::insert;

typedef std::allocator<Elem_00434020> Alloc_004343f0;
typedef void (Alloc_004343f0::*DestroyFn_004343f0)(Elem_00434020*);

// FUNCTION: 0x4343f0 ?destroy@?$allocator@UElem_00434020@@@std@@QAEXPAUElem_00434020@@@Z
DestroyFn_004343f0 g_destroy_004343f0 = &Alloc_004343f0::destroy;

typedef std::allocator<Elem_00434360> Alloc_00434400;
typedef void (Alloc_00434400::*DestroyFn_00434400)(Elem_00434360*);

// FUNCTION: 0x434400 ?destroy@?$allocator@UElem_00434360@@@std@@QAEXPAUElem_00434360@@@Z
DestroyFn_00434400 g_destroy_00434400 = &Alloc_00434400::destroy;

// FUNCTION: 0x434430
void __stdcall DestroyPoint(int)
{
}

// FUNCTION: 0x434440
void __stdcall DestroyLine(Elem_00434360* p)
{
    std::_Destroy(p);
}

typedef std::vector<Elem_00434360> Vec_00434470;
typedef void (Vec_00434470::*InsertFn_00434470)(
    Vec_00434470::iterator, Vec_00434470::size_type, const Elem_00434360&);

typedef std::vector<Elem_00434360> Vec_004349f0;
typedef Vec_004349f0& (Vec_004349f0::*AssignFn_004349f0)(const Vec_004349f0&);

// FUNCTION: 0x434470 ??0Elem_00434360@@QAE@ABU0@@Z
InsertFn_00434470 g_insert_00434470 = &Vec_00434470::insert;

// FUNCTION: 0x4349f0 ??_GElem_00434360@@QAEPAXI@Z
AssignFn_004349f0 g_assign_004349f0 = &Vec_004349f0::operator=;

// FUNCTION: 0x4345c0
void __stdcall StoreDwordIfDst(int* param_1, int* param_2)
{
    if (param_1 != 0) {
        *param_1 = *param_2;
    }
}

typedef std::vector<Elem_00434020> Inner_004345e0;
typedef Inner_004345e0& (Inner_004345e0::*AssignFn_004345e0)(const Inner_004345e0&);

// FUNCTION: 0x4345e0 ??4?$vector@UElem_00434020@@V?$allocator@UElem_00434020@@@std@@@std@@QAEAAV01@ABV01@@Z
AssignFn_004345e0 g_assign_004345e0 = &Inner_004345e0::operator=;

typedef std::vector<Elem_00434360> Vec_00434770;
typedef Vec_00434770& (Vec_00434770::*AssignFn_00434770)(const Vec_00434770&);

// FUNCTION: 0x434770 ??4?$vector@UElem_00434360@@V?$allocator@UElem_00434360@@@std@@@std@@QAEAAV01@ABV01@@Z
AssignFn_00434770 g_assign_00434770 = &Vec_00434770::operator=;

// FUNCTION: 0x433500
Inner_00433500* LosTables::GetLosTable(int n)
{
    return (Inner_00433500*)&(*(Outer_004330b0*)this)[(short)(n - 1)];
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
                Outer_004330b0& tables = *(Outer_004330b0*)this;
                Column_00433270 temp;
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
    Column_00433270 x;
    ((Outer_004330b0*)this)->resize(n, x);
}

typedef Outer_004330b0& (Outer_004330b0::*AssignFn_00433a80)(const Outer_004330b0&);

AssignFn_00433a80 g_assign_00433a80 = &Outer_004330b0::operator=;

// FUNCTION: 0x433a80 ??1?$vector@UElem_00434360@@V?$allocator@UElem_00434360@@@std@@@std@@QAE@XZ

typedef Outer_00433a30::iterator (Outer_00433a30::*EraseFn_00434020)(
    Outer_00433a30::iterator, Outer_00433a30::iterator);

// FUNCTION: 0x434020 ?erase@?$vector@UElem_00434360@@V?$allocator@UElem_00434360@@@std@@@std@@QAEPAUElem_00434360@@PAU3@0@Z
EraseFn_00434020 g_erase_00434020 = &Outer_00433a30::erase;

// FUNCTION: 0x4344e0 ??0?$vector@UElem_00434360@@V?$allocator@UElem_00434360@@@std@@@std@@QAE@ABV01@@Z
InsertFn_004340f0 g_insert_004344e0 = &Outer_004340f0::insert;

typedef Outer_004330b0::iterator (Outer_004330b0::*EraseFn_00434360)(
    Outer_004330b0::iterator, Outer_004330b0::iterator);

// FUNCTION: 0x434360 ?erase@?$vector@V?$vector@UElem_00434360@@V?$allocator@UElem_00434360@@@std@@@std@@V?$allocator@V?$vector@UElem_00434360@@V?$allocator@UElem_00434360@@@std@@@std@@@2@@std@@QAEPAV?$vector@UElem_00434360@@V?$allocator@UElem_00434360@@@std@@@2@PAV32@0@Z
EraseFn_00434360 g_erase_00434360 = &Outer_004330b0::erase;
