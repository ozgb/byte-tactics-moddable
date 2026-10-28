#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "../weapons/unit_weapon_slot.h"

struct Item_00485940;

#pragma pack(push, 1)
#include "../util/angles.h"

#include "../network/player.h"
#include "../util/vec3.h"
#include "../../interfaces/seams.h"

struct UnitDefMaxHealthView {
    char unknown_0[0x1fa];
    int f1fa;
};

#include "unit.h"

struct UnitPtrVectorView {
    char allocator_byte;
    char pad[3];
    Unit** ids;
};
#pragma pack(pop)

#include "../orders/mission_type.h"

#include "unit_commands.h"

union MissionSlot {
    MissionType type;
    int slot;
    MissionSlot() {}
};

struct Outs_t {
    MissionSlot g, a, m, u, p;
};

// FUNCTION: 0x487bf0
void __stdcall RunInitialMission(Unit* unit, char* text, UnitPtrVectorView* table)
{
    int count;
    struct { float f1, f2; int n; Vec3 pos; int move; } L;
    Outs_t out;
    char buf[256];
    int processed = 0, selected = 0;
    struct { float wf; float pf; int fire; } M2;

    while (*text != 0) {
        if (isspace(*text)) {
            do
                text += 1;
            while (isspace(*text));
        }
        L.n = strcspn(text, ",");
        strncpy(buf, text, L.n);
        text += L.n;
        buf[L.n] = 0;
        if (*text == ',')
            text = text + 1;

        switch (buf[0]) {
        case 'O':
        case 'o': {
            unsigned int* flags = &unit->flags;
            L.move = (unit->flags >> 0x12) & 3;
            M2.fire = (int)((*flags >> 0x14) & 3);
            sscanf(buf + 1, " %d %d", &L.move, &M2.fire);
            unit->flags = (0xffc3ffff & unit->flags)
                          | ((((3 & M2.fire) << 2) | (3 & L.move)) << 0x12);
            break;
        }
        case 'M':
        case 'm': {
            sscanf(buf + 1, " %f %f", &L.f1, &L.f2);
            L.pos.x = (int)(L.f1 * 65536.0);
            L.pos.y = 0;
            L.pos.z = (int)(L.f2 * 65536.0);
            out.m.type = g_seams.orders->GetOrderType(2, unit, 0, (Vec3*)&L.pos);
            g_seams.orders->AddOrder(out.m.type, 1, unit, 0, (Vec3*)&L.pos, 0, 0);
            processed = 1;
            break;
        }
        case 'U':
        case 'u': {
            sscanf(buf + 1, " %f %f", &L.f1, &L.f2);
            L.pos.x = (int)(L.f1 * 65536.0);
            L.pos.y = 0;
            L.pos.z = (int)(65536.0 * L.f2);
            out.u.type = g_seams.orders->GetOrderType(5, unit, 0, (Vec3*)&L.pos);
            g_seams.orders->AddOrder(out.u.type, 1, unit, 0, (Vec3*)&L.pos, 0, 0);
            processed = 1;
            break;
        }
        case 'G':
        case 'g': {
            sscanf(buf + 1, " %[a-zA-Z0-9_.]", buf);
            int target = g_seams.units->FindMissionUnit(buf, (std::vector<Unit*, std::allocator<Unit*> >*)table, 0);
            if (target != 0) {
                out.g.type = g_seams.orders->GetOrderType(7, unit, (Unit*)target, 0);
                g_seams.orders->AddOrder(out.g.type, 1, unit, (Unit*)target, 0, 0, 0);
                processed = 1;
            }
            break;
        }
        case 'P':
        case 'p': {
            M2.pf = 0.0f;
            sscanf(buf + 1, " %f %f %f", &L.f1, &L.f2, &M2.pf);
            L.pos.x = (int)(L.f1 * 65536.0);
            L.pos.y = 0;
            L.pos.z = (int)(L.f2 * 65536.0);
            out.p.type = g_seams.orders->GetOrderType(9, unit, 0, (Vec3*)&L.pos);
            g_seams.orders->AddOrder(out.p.type, 1, unit, 0, (Vec3*)&L.pos, (int)(M2.pf * 30.0f), 0);
            processed = 1;
            selected = 1;
            break;
        }
        case 'A':
        case 'a': {
            if (sscanf(buf + 1, " %f %f", &L.f1, &L.f2) == 2) {
                L.pos.x = (int)(L.f1 * 65536.0);
                L.pos.y = 0;
                L.pos.z = (int)(L.f2 * 65536.0);
                out.a.type = g_seams.orders->GetOrderType(3, unit, 0, (Vec3*)&L.pos);
                g_seams.orders->AddOrder(out.a.type, 1, unit, 0, (Vec3*)&L.pos, 0, 0);
                selected = 1;
                processed = 1;
            } else {
                sscanf(buf + 1, " %[a-zA-Z0-9_.]", buf);
                unsigned short id = g_seams.units->FindUnitTypeId(buf);
                if (id != 0) {
                    g_seams.orders->AddOrder("ATTACKUTYPE", 1, unit, 0, 0, id, 0);
                    processed = 1;
                }
            }
            break;
        }
        case 'B':
        case 'b': {
            L.n = 1;
            if (buf[1] == 'w' || buf[1] == 'W') {
                sscanf(buf + 2, " %d", &L.n);
                g_seams.orders->AddOrder("BUILDWEAPON", 1, unit, 0, 0, 0, L.n);
            } else {
                sscanf(buf + 1, " %[a-zA-Z0-9_.] %d %f %f", buf, &L.n, &L.f1, &L.f2);
                L.pos.x = (int)(L.f1 * 65536.0);
                L.pos.y = 0;
                L.pos.z = (int)(65536.0 * L.f2);
                unsigned short id = g_seams.units->FindUnitTypeId(buf);
                if (id != 0) {
                    if (unit->motion != 0)
                        g_seams.orders->AddOrder("MOBILEBUILD", 1, unit, 0,
                                     (Vec3*)&L.pos, id, L.n);
                    else
                        g_seams.orders->AddOrder("BUILDINGBUILD", 1, unit, 0,
                                     0, id, L.n);
                    processed = 1;
                }
            }
            break;
        }
        case 'W':
        case 'w':
            if (buf[1] == 'a' || buf[1] == 'A') {
                count = sscanf(buf + 2, " %[a-zA-Z0-9.]", buf);
                int target = 0;
                if (count == 1)
                    target = g_seams.units->FindMissionUnit(buf, (std::vector<Unit*, std::allocator<Unit*> >*)table, 0);
                if (target == 0)
                    target = (int)unit;
                g_seams.orders->AddOrder("WAITFORATTACK", 1, unit, (Unit*)target,
                             0, 0, 0);
                processed = 1;
            } else {
                M2.wf = 0.0f;
                L.move = 0;
                sscanf(buf + 1, " %f %d", &M2.wf, &L.move);
                g_seams.orders->AddOrder("WAIT", 1, unit, 0, 0, (int)(M2.wf * 30.0f), L.move);
                processed = 1;
            }
            break;
        case 'D':
        case 'd':
            g_seams.orders->AddOrder("SELFDESTRUCTFG", 1, unit, 0, 0, 1, 0);
            processed = 1;
            selected = 1;
            break;
        case 'S':
        case 's':
            g_seams.orders->AddOrder("MAKESELECTABLE", 1, unit, 0, 0, 0, 0);
            processed = 1;
            selected = 1;
            break;
        case 'I':
        case 'i': {
            sscanf(buf + 1, " %[a-zA-Z0-9_.]", buf);
            int target = g_seams.units->FindMissionUnit(buf, (std::vector<Unit*, std::allocator<Unit*> >*)table, 0);
            if (target != 0)
                g_seams.units->AttachUnitToPiece(unit, (Unit*)target, -1, 0);
            break;
        }
        }
    }
    if (processed) {
        unit->flags &= ~0x20u;
        if (selected == 0)
            g_seams.orders->AddOrder("MAKESELECTABLE", 1, unit, 0, 0, 0, 0);
    }
}

#pragma pack(push, 1)
struct UnitDefNameKeyView {
    char unknown_0[0x20];
    char name[0x21e - 0x20];
    unsigned short id;
    char unknown_220[0x249 - 0x220];
};

#include "../map/mission.h"
#include "../game/mission_conditions.h"

#include "../game/game.h"
#pragma pack(pop)

#include "../../link/globals.h"

UnitDefNameKeyView* __stdcall FindUnitType(const char* name);
Unit* __stdcall CreateUnit(unsigned char player, unsigned short id,
                                      Vec3 pos, int a, int b, int c);
#include "../graphics/display.h"

#include <math.h>
#include <memory.h>
#include <vector>

namespace std {
template<> class vector<Unit*, allocator<Unit*> > {
public:
    typedef Unit* T;
    typedef allocator<Unit*> A;
    typedef A::size_type size_type;

    vector(size_type _N, const T& _V = T(), const A& _Al = A())
        : allocator(_Al)
    {
        _First = allocator.allocate(_N, (void *)0);
        _Ufill(_First, _N, _V);
        _Last = _First + _N;
        _End = _Last;
    }
    ~vector()
        { allocator.deallocate(_First, _End - _First); }
    T& operator[](size_type _P)
        { return (*(_First + _P)); }

protected:
    A allocator;
    T* _First;
    T* _Last;
    T* _End;
    void _Ufill(T* _F, size_type _N, const T& _X);
};
}

// FUNCTION: 0x488310
void __cdecl CreateMissionUnits()
{
    Player* p;
    char buf[100];
    int n = g_game->net->unitCount;
    std::vector<Unit*> units(n);
    for (int i = 0; i < g_game->net->unitCount; i++) {
        MissionUnit* e = &g_game->net->units[i];
        UnitDefNameKeyView* item = (UnitDefNameKeyView*)g_seams.units->FindUnitType(e->name);
        if (item == 0) {
            units[i] = 0;
            continue;
        }
        unsigned char pl = (unsigned char)(e->player - 1);
        if (pl >= 10
            || (p = &g_game->players[pl], g_game->players[pl].active == 0)
            || (p->type != 1 && p->type != 2 && p->type != 3)
            || p->index == 10) {
            sprintf(buf, "Player number %d invalid for unit %s", e->player, e->name.get());
            FatalError(buf);
        }
        g_seams.map->SnapWorldPosToFootprint((UnitDef*)item, (Vec3*)&e->x);
        Unit* u = g_seams.units->CreateUnit((unsigned char)(e->player - 1), item->id, *(Vec3*)&e->x, 1, 1, 0);
        if (u) {
            u->flags = (u->flags & ~0x8000) | ((e->flags & 0xffffff80) << 8);
            u->health = (unsigned short)((unsigned)(((UnitDefMaxHealthView*)u->def.get())->f1fa * e->health) / 100);
            u->angles.heading = e->angle;
            units[i] = u;
        }
    }
    for (int j = 0; j < g_game->net->unitCount; j++) {
        MissionUnit* e = &g_game->net->units[j];
        if (e->initialMission && units[j])
            RunInitialMission(units[j], e->initialMission, (UnitPtrVectorView*)&units);
    }
    if (g_game->net->unitCount <= 0)
        g_seams.players->DeactivateConditions(g_game->list_391ed);
}

#include "../network/net_game.h"
void __stdcall BroadcastPacket(int who, OwnershipTransferPacket* packet, int size);

// FUNCTION: 0x488570
void __stdcall GiveUnitToPlayer(Unit* unit, Player* other, OwnershipTransferPacket* p)
{
    if (unit->player == other)
        return;
    if (!(unit->flags & 0x10000000))
        return;
    if (unit->flags & 0x4000)
        return;
    g_game->list_391ed->NotifyUnitCaptured(unit);

    Player* cur = unit->player;
    if (cur->active != 0 && (cur->type == 1 || cur->type == 2)) {
        if (other->active == 0)
            return;
        if (other->type == 3) {
            OwnershipTransferPacket pk;
            unit->flags &= ~0x10;
            unit->postTransferHoldoff = 0x96;
            pk.type = 0x14;
            pk.newOwnerNetId = GetPlayerDpid(other);
            pk.unitId = unit->id;
            pk.buildPercent = (int)unit->buildLeft;
            pk.hitPoints = unit->health;
            pk.angles = unit->angles;
            unsigned char a = unit->weapons[0].flags & 2;
            pk.stockpile0 = a ? unit->weapons[0].stockpile : 0;
            pk.stockpile1 = a ? unit->weapons[1].stockpile : 0;
            pk.stockpile2 = a ? unit->weapons[2].stockpile : 0;
            BroadcastPacket(unit->player->id, &pk, 0x18);
            g_seams.weapons->DamageUnit(0, unit, 30000, 4, 0);
            return;
        }
    }
    if (other->active == 0)
        return;
    if (other->type != 1 && other->type != 2)
        return;

    Unit* n = g_seams.units->CreateUnit(other->index, unit->unitDefIndex, *(Vec3*)&unit->pos, 1, unit->flags & 3, 0);
    if (!n)
        return;
    n->flags &= 0xffc3ffff;
    if (p) {
        n->health = (short)p->hitPoints;
        n->buildLeft = (float)p->buildPercent;
        n->angles = p->angles;
        if (n->weapons[0].flags & 2)
            n->weapons[0].stockpile = p->stockpile0;
        if (n->weapons[1].flags & 2)
            n->weapons[1].stockpile = p->stockpile1;
        if (n->weapons[2].flags & 2)
            n->weapons[2].stockpile = p->stockpile2;
    } else {
        n->health = unit->health;
        n->speed_bits = unit->speed_bits;
        n->angles = unit->angles;
        if (n->weapons[0].flags & 2)
            n->weapons[0].stockpile = unit->weapons[0].stockpile;
        if (n->weapons[1].flags & 2)
            n->weapons[1].stockpile = unit->weapons[1].stockpile;
        if (n->weapons[2].flags & 2)
            n->weapons[2].stockpile = unit->weapons[2].stockpile;
        g_seams.weapons->DamageUnit(0, unit, 30000, 4, 0);
    }
    g_seams.units->SetStateBits(n, unit->activateFlags, 1);
    g_seams.units->SetStateBits(n, ~unit->activateFlags, 0);
}

typedef int (__stdcall* QuickSortPred)(Item_00485940*, Item_00485940*);

static inline int PointerSpanBytes(Item_00485940** low, Item_00485940** high)
{
    return (int)((char*)high - (char*)low) & ~3;
}

void __stdcall QuickSort(Item_00485940** first, Item_00485940** last, QuickSortPred pred, int unused);

// FUNCTION: 0x488810
void __stdcall QuickSort(Item_00485940** first, Item_00485940** last, QuickSortPred pred, int unused)
{
    while (((int)((char*)last - (char*)first) & ~3) > 0x40) {
        int len = (int)((char*)last - (char*)first);
        Item_00485940* a = *first;
        Item_00485940* m = first[(len >> 2) / 2];
        Item_00485940* c = last[-1];
        Item_00485940* pivot;

        if (pred(a, m)) {
            if (pred(m, c))
                pivot = m;
            else if (pred(a, c))
                pivot = c;
            else
                pivot = a;
        } else if (pred(a, c)) {
            pivot = a;
        } else {
            pivot = pred(m, c) ? c : m;
        }

        Item_00485940** j = last;
        Item_00485940** i = first;
        for (;;) {
            while (pred(*i, pivot))
                ++i;
            while (pred(pivot, *--j))
                ;
            if (j <= i)
                break;
            Item_00485940* t = *i;
            *i = *j;
            *j = t;
            ++i;
        }

        if (PointerSpanBytes(i, last) <= PointerSpanBytes(first, i)) {
            QuickSort(i, last, pred, 0);
            last = i;
        } else {
            QuickSort(first, i, pred, 0);
            first = i;
        }
    }
}

// FUNCTION: 0x488920
void __stdcall InsertShift(Item_00485940** last, Item_00485940* value, QuickSortPred pred)
{
    for (Item_00485940** m = last; pred(value, *--m); last = m)
        *last = *m;
    *last = value;
}

// FUNCTION: 0x488960
Item_00485940** __stdcall Partition(Item_00485940** first, Item_00485940** last,
                                        Item_00485940* pivot, QuickSortPred pred)
{
    for (;; ++first) {
        for (; pred(*first, pivot); ++first)
            ;
        for (; pred(pivot, *--last);)
            ;
        if (last <= first)
            return first;
        Item_00485940* t = *first;
        *first = *last;
        *last = t;
    }
}

#include "../util/string_ref.h"
#include "../state/sim_globals.h"

class UnitCategory {
public:
    StringRef name;
    void* value;

    UnitCategory(const StringRef& n, void* v) : name(n) { value = v; }
    UnitCategory(const UnitCategory& other);
    UnitCategory& operator=(const UnitCategory& other)
    {
        AssignCategory((int*)&other);
        return *this;
    }
    void* AssignCategory(int* other);
};


// FUNCTION: 0x4889d0 _$E5
// FUNCTION: 0x488a00 _$E3
static std::vector<UnitCategory> s_unitCategories;

extern "C" int __cdecl _strcmpi(const char* str1, const char* str2);

static inline int NameLessNoCase(const char* a, const char* b)
{
    return _strcmpi(a, b) < 0;
}

#pragma auto_inline(off)
// FUNCTION: 0x488a50
UnitDefNameKeyView* __stdcall FindUnitType(const char* name)
{
    UnitDefNameKeyView* end = (UnitDefNameKeyView *)g_game->defs.get() + g_game->unitDefCount;
    UnitDefNameKeyView* lo = (UnitDefNameKeyView *)g_game->defs.get() + 1;
    int n = (int)(end - lo);
    while (n > 0) {
        int half = n / 2;
        UnitDefNameKeyView* mid = lo + half;
        if (NameLessNoCase(mid->name, name)) {
            lo = mid + 1;
            n = n - half - 1;
        } else {
            n = half;
        }
    }
    if (lo != end && _strcmpi(name, lo->name) == 0)
        return lo;
    return 0;
}
#pragma auto_inline(on)

// FUNCTION: 0x488b10
unsigned short __stdcall FindUnitTypeId(const char* name)
{
    UnitDefNameKeyView* last = (UnitDefNameKeyView *)g_game->defs.get() + g_game->unitDefCount;
    UnitDefNameKeyView* first = (UnitDefNameKeyView *)g_game->defs.get() + 1;
    int n = ((char*)last - (char*)first) / 0x249;
    while (n > 0) {
        int mid = n / 2;
        UnitDefNameKeyView* e = first + mid;
        if (_strcmpi(e->name, name) < 0) {
            first = e + 1;
            n = n - mid - 1;
        } else {
            n = mid;
        }
    }
    if (first == last || _strcmpi(name, first->name) != 0)
        first = 0;
    if (first)
        return first->id;
    return 0;
}


// FUNCTION: 0x488be0
void InitUnitCategories()
{
    g_mapLoadFlag = 1;
}

// FUNCTION: 0x488bf0
void FreeUnitCategories()
{
    for (std::vector<UnitCategory>::iterator it = s_unitCategories.begin(); it != s_unitCategories.end(); it++)
        delete it->value;
    s_unitCategories.clear();
    g_mapLoadFlag = 0;
}

#include "unit_type_set.h"

// FUNCTION: 0x488c50
UnitTypeSet* __stdcall GetCategoryMask(char* name)
{
    UnitCategory* first = s_unitCategories.begin();
    int n = s_unitCategories.end() - first;
    for (; 0 < n; ) {
        int n2 = n / 2;
        UnitCategory* m = first + n2;
        if (_strcmpi(m->name.data, name) < 0)
            first = ++m, n -= n2 + 1;
        else
            n = n2;
    }
    if (first != s_unitCategories.end() && _strcmpi(first->name.data, name) == 0)
        return (UnitTypeSet*)first->value;

    UnitTypeSet* p = new UnitTypeSet;
    s_unitCategories.insert(first, UnitCategory(StringRef(name), p));
    return p;
}

static inline UnitDefNameKeyView* FindByName(UnitDefNameKeyView* first, UnitDefNameKeyView* last, char* text)
{
    int n = (int)((char*)last - (char*)first) / 0x249;
    while (n > 0) {
        int m = n / 2;
        UnitDefNameKeyView* e = first + m;
        if (_strcmpi(e->name, text) < 0) {
            n = n - m - 1;
            first = e + 1;
        } else {
            n = m;
        }
    }
    if (first == last || _strcmpi(text, first->name) != 0)
        return 0;
    return first;
}

// FUNCTION: 0x488d30
void UnitTypeSet::AddTypeOrCategory(char* text, int* out)
{
    UnitDefNameKeyView* base = (UnitDefNameKeyView *)g_game->defs.get();
    UnitDefNameKeyView* last = base + g_game->unitDefCount;
    UnitDefNameKeyView* first = FindByName(base + 1, last, text);
    unsigned short v = first ? first->id : 0;
    if (v != 0) {
        bits[v >> 5] |= 1 << (v & 31);
        *out = 1;
        return;
    }
    UnitTypeSet* other = GetCategoryMask(text);
    for (int i = 0; i < 16; i++)
        bits[i] |= other->bits[i];
    *out = 0;
}

struct UnitDef;
#include "../units/unit_def.h"

// FUNCTION: 0x488f30
float __stdcall GetEnergyUse(UnitDef* def)
{
    if (def->energyuse != 0.0) {
        return def->energyuse;
    }
    if (def->windgenerator > 0.0f) {
        return -(g_game->windStrength * def->windgenerator);
    }
    if (def->tidalgenerator > 0.0f) {
        return -(g_game->tidal * def->tidalgenerator);
    }
    return 0.0f;
}

typedef std::vector<UnitCategory> UnitCategoryVector;
typedef void (UnitCategoryVector::*UnitCategoryInsertFn)(
    UnitCategoryVector::iterator, UnitCategoryVector::size_type, const UnitCategory&);

// FUNCTION: 0x488fb0 ?insert@?$vector@VUnitCategory@@V?$allocator@VUnitCategory@@@std@@@std@@QAEXPAVUnitCategory@@IABV3@@Z
UnitCategoryInsertFn g_insert_00488fb0 = &UnitCategoryVector::insert;

#pragma auto_inline(off)
// FUNCTION: 0x489240
void* UnitCategory::AssignCategory(int* other)
{
    name.Assign((StringRef*)other);
    value = (void*)other[1];
    return this;
}

#pragma auto_inline(on)

// FUNCTION: 0x489260
UnitCategory::UnitCategory(const UnitCategory& other)
    : name(other.name), value(other.value)
{
}
