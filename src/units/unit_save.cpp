#include <stdio.h>
#include "../state/block.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>

struct FlagBits {
    unsigned int a0 : 1;
    unsigned int a1 : 1;
    unsigned int a2 : 1;
    unsigned int a3 : 1;
    unsigned int b : 12;
    unsigned int c : 1;
    unsigned int d : 3;
    unsigned int e : 12;
};



#include "../util/vec3.h"

struct PieceBits {
    unsigned char b0 : 1;
    unsigned char b1 : 1;
    unsigned char b23 : 2;
    unsigned char b4 : 1;
    unsigned char b5 : 3;
};

#include "../util/angles.h"

#pragma pack(push, 1)

struct SrcPiece {
    int f0;
    int f4;
    union {
        int f8;
        unsigned char f8b;
    };
    int fc;
    short f10;
    short f12;
    short f14;
    unsigned char f16;
    PieceBits fl;
};

struct SaveRec {
    char name[0x20];
    unsigned char player;
    unsigned short id;
    int f23;
    int f27;
    Vec3 pos;
    Angles16 angles;
    short f3d;
    short f3f;
    SrcPiece pieces[3];
    short childA;
    short childB;
    char b8d;
    unsigned char b8e;
    int f8f;
    int f93;
    int f97;
    int f9b;
    int f9f;
    int fa3;
    int fa7;
    unsigned char bab;
    unsigned char bac;
    unsigned char bad;
    short bae;
    unsigned char bb0;
    unsigned char bb1;
    union {
        unsigned short fb2;
        struct {
            unsigned char bb2;
            unsigned char b3;
        };
    };
    FlagBits flags;
};



#include "unit.h"

struct Mission;

#include "../game/game.h"

#include "../map/mission.h"

struct Struct_00487af0 {
    char unknown_0[4];
    int* ids;
};
#pragma pack(pop)

#include "../files/files_api.h"

#include "cob_script.h"

#include "../orders/unit_motion.h"

#pragma pack(push, 1)
#include "../orders/order.h"

#pragma pack(pop)

#include "../game/unit_resources.h"

#include "../../link/globals.h"

#include "unit_save.h"
#include "unit_commands.h"
Unit* __stdcall CreateUnit(unsigned char player, unsigned short typeId,
                                      Vec3 pos, int param_5, int mode,
                                      unsigned short id);
void __stdcall AttachUnitToPiece(Unit* unit, Unit* builder, int piece, int p4);

// FUNCTION: 0x486fd0
void __stdcall LoadUnits(HapiBank* file)
{
    if (file->OpenAccount("Units")) {
        if (file->GetIntegerItem("Version", 0) == 0x11) {
            int n = file->GetIntegerItem("Number of Units", 0);
            for (int i = 0; i < n; i++) {
                if (file->OpenNumberedBox(i)) {
                    file->SeekBox(0);
                    SaveRec rec;
                    int len = file->ReadBox(&rec, 0xb8);
                    if (len != 0xb8) {
                        if (len + 2 != 0xb8)
                            continue;
                        rec.id = 0;
                    }
                    LoadUnit(rec.id, file);
                }
            }
        }
    }
}

#include "../../interfaces/seams.h"

// FUNCTION: 0x487080
Unit* __stdcall LoadUnit(unsigned short id, HapiBank* file)
{
    Unit* unit;
    if (id == 0)
        unit = 0;
    else
        unit = (Unit*)((char*)g_game->units.get() + id * 0x118);
    if (unit == 0 || (unit->flags & 0x10000000))
        return unit;

    SaveRec rec;
    char name[32];
    char script[32];
    int n = file->GetIntegerItem("Number of Units", 0);
    int found = 0;
    int i;
    for (i = 0; i < n; i++) {
        if (!file->OpenNumberedBox(i))
            return 0;
        file->SeekBox(0);
        if (file->ReadBox(&rec, 0xb8) != 0xb8)
            return 0;
        if (rec.id == id) {
            found = 1;
            break;
        }
    }
    if (!found)
        return 0;

    unit = CreateUnit(rec.player, FindUnitTypeId(rec.name), rec.pos, 1, rec.flags.b & 3, rec.id);
    if (unit != 0) {

    unit->angles = rec.angles;
    unit->health = rec.f3d;
    unit->killCount = rec.f3f;
    unit->pos.y = rec.pos.y;

    if (rec.childA != 0) {
        Unit* child = LoadUnit(rec.childA, file);
        if (child != 0)
            AttachUnitToPiece(unit, child, rec.b8d, rec.flags.b & 3);
    }
    unit->attacker = LoadUnit(rec.childB, file);
    unit->transportPiece = rec.b8d;
    unit->lastAttackerSlot = rec.b8e;
    unit->extraction = rec.f8f;
    *(int*)&unit->cell = rec.f93;
    unit->losCacheCell = rec.f97;
    *(int*)&unit->footprint = rec.f9b;
    unit->group = rec.f9f;
    g_seams.ai->SetUnitSquad(unit, rec.f9f);
    unit->speed_bits = rec.fa7;
    unit->lastDamageType = rec.bab;
    unit->healthPercent = rec.bac;
    unit->prevHealthPercent = rec.bad;
    unit->netDirtyFlags = rec.bae;
    unit->losSightFrameIdx = rec.bb0;
    unit->recentlyDamagedTimer = rec.bb1;
    unit->activateFlags = rec.bb2;

    unit->bit0 = rec.flags.a0;
    unit->bit1 = rec.flags.a1;
    unit->bit2 = rec.flags.a2;
    unit->bit3 = rec.flags.a3;
    unit->flags = (unit->flags & ~0xc) | (rec.flags.b & 0xc);
    unit->flags = (unit->flags & ~0x10) | (rec.flags.b & 0x10);
    unit->flags = (unit->flags & ~0x20) | (rec.flags.b & 0x20);
    unit->flags = (unit->flags & ~0xc0) | (rec.flags.b & 0xc0);
    unit->flags = (unit->flags & ~0x100) | (rec.flags.b & 0x100);
    unit->flags = (unit->flags & ~0x200) | (rec.flags.b & 0x200);
    unit->flags = (unit->flags & ~0x400) | (rec.flags.b & 0x400);
    unit->flags = (unit->flags & ~0x800) | (rec.flags.b & 0x800);
    unit->workTime = rec.fa3;
    unit->flags = (unit->flags & ~0x2000) | (rec.flags.c << 13);
    unit->flags = (unit->flags & ~0x4000) | ((rec.flags.e << 14) & 0x4000);
    unit->flags = (unit->flags & ~0x8000) | ((rec.flags.e << 14) & 0x8000);
    unit->flags = (unit->flags & ~0x10000) | ((rec.flags.e << 14) & 0x10000);
    unit->flags = (unit->flags & ~0x20000) | ((rec.flags.e << 14) & 0x20000);
    unit->flags = (unit->flags & ~0xc0000) | ((rec.flags.e << 14) & 0xc0000);
    unit->flags = (unit->flags & ~0x300000) | ((rec.flags.e << 14) & 0x300000);
    unit->flags = (unit->flags & ~0x400000) | ((rec.flags.e << 14) & 0x400000);
    unit->flags = (unit->flags & ~0x3800000) | ((rec.flags.e << 14) & 0x3800000);

    g_seams.economy->LoadUnitAccounts(&unit->resourceSlot, (UnitInfo*)unit, file);
    if (rec.f27 != 0)
        g_seams.orders->LoadMotion(unit->motion, 0, unit, file);

    Ref<Order>* normal = &unit->list;
    Ref<Order>* special = &unit->list2;
    int k = 0;
    if (rec.f23 > 0) {
        do {
            sprintf(name, "u%04xm%04x", unit->id, k);
            Order* p = new (kSim) Order(unit, file, name);
            if (p->flags_42 & 0x40000) {
                *special = p;
                special = &p->next;
            } else {
                *normal = p;
                normal = &p->next;
            }
            k++;
        } while (k < rec.f23);
    }
    if (unit->list != 0)
        g_seams.orders->ReattachFxToUnit((Order*)unit->list);
    sprintf(script, "Script%i", i);
    file->OpenNamedBox(script);
    ((CobScript*)unit->script.get())->LoadScriptState(file);

    for (int j = 0; j < 3; j++) {
        *(int*)&unit->weapons[j] = rec.pieces[j].f0;
        unit->weapons[j].aimCob.nAimCobDone = rec.pieces[j].f4;
        ((unsigned char*)unit->weapons[j].weapon.get())[0x10a] = rec.pieces[j].f8b;
        unit->weapons[j].muzzleAimFromDeltaZ = rec.pieces[j].fc;
        unit->weapons[j].reloadTimer = rec.pieces[j].f10;
        unit->weapons[j].fireHeading = rec.pieces[j].f12;
        unit->weapons[j].firePitch = rec.pieces[j].f14;
        unit->weapons[j].stockpile = rec.pieces[j].f16;
        ((PieceBits*)&unit->weapons[j].flags)->b0 = rec.pieces[j].fl.b0;
        ((PieceBits*)&unit->weapons[j].flags)->b1 = rec.pieces[j].fl.b1;
        ((PieceBits*)&unit->weapons[j].flags)->b23 = rec.pieces[j].fl.b23;
        ((PieceBits*)&unit->weapons[j].flags)->b4 = rec.pieces[j].fl.b4;
    }

    if (unit->cobStateFlags & 4)
        g_seams.map->ForceNeighborFootprintReclaim(unit);
    return unit;
    }
    return 0;
}

// FUNCTION: 0x4876c0
void __stdcall SaveUnits(HapiBank* file)
{
    int count = 0;
    Unit* end = 0;
    Unit* unit;
    file->OpenAccount("Units");
    end = g_game->unitsEnd.get();
    unit = g_game->units.get();
    for (; unit <= end; unit = (Unit*)((char*)unit + 0x118)) {
        if (unit->flags & 0x10000000) {
            SaveRec rec;
            char bufTail[32], script[32];
            char bufHead[32];

            sprintf(script, "Script%i", count);
            file->OpenNamedBox(script);
            ((CobScript*)unit->script.get())->SaveScriptState(file);

            int n = 0;
            Order* c = (Order*)unit->list;
            while (c != 0) {
                sprintf(bufHead, "u%04xm%04x", unit->id, n);
                g_seams.orders->SerializeOrderToSave(c, 0, unit, file, bufHead);
                c = c->next;
                n++;
            }
            c = (Order*)unit->list2;
            while (c != 0) {
                sprintf(bufTail, "u%04xm%04x", unit->id, n);
                g_seams.orders->SerializeOrderToSave(c, 0, unit, file, bufTail);
                c = c->next;
                n++;
            }

            if (unit->motion != 0)
                g_seams.orders->SaveMotion(unit->motion, 0, unit, file);
            g_seams.economy->SaveUnitAccounts((UnitResources*)((char*)unit + 0xbc), (UnitInfo*)unit, file);

            strcpy(rec.name, ((char*)unit->def.get() + 0x20));
            rec.player = unit->playerIndex;
            rec.id = unit->id;

            rec.pos = unit->pos;
            rec.angles = unit->angles;
            rec.f23 = n;
            rec.f3d = unit->health;
            rec.f3f = unit->killCount;
            rec.f27 = unit->motion != 0;

            short id8b = 0;
            Unit* a = unit->carrier;
            if (a != 0 && (a->flags & 0x10000000)) {
                rec.childA = unit->carrier == 0 ? 0 : a->id;
                rec.b8d = unit->transportPiece;
            } else {
                rec.childA = 0;
                rec.b8d = 0xff;
            }
            Unit* a2 = (Unit*)unit->attacker;
            if (a2 != 0) {
                if (a2->flags & 0x10000000)
                    id8b = unit->attacker == 0 ? 0 : a2->id;
            }
            rec.childB = id8b;
            rec.f93 = *(int*)&unit->cell;
            rec.b8e = unit->lastAttackerSlot;
            rec.f8f = unit->extraction;
            rec.f9f = unit->group;
            rec.f97 = unit->losCacheCell;
            rec.f9b = *(int*)&unit->footprint;
            rec.bac = unit->healthPercent;
            rec.fa7 = unit->speed_bits;
            rec.bab = unit->lastDamageType;
            rec.bb0 = unit->losSightFrameIdx;
            rec.bad = unit->prevHealthPercent;
            rec.bae = unit->netDirtyFlags;
            rec.fa3 = unit->workTime;
            rec.bb1 = unit->recentlyDamagedTimer;
            rec.fb2 = unit->activateFlags;
            unsigned int u = unit->flags;
            rec.flags.a0 = unit->bit0;
            rec.flags.a1 = unit->bit1;
            rec.flags.a2 = unit->bit2;
            rec.flags.a3 = unit->bit3;
            rec.flags.b = u & 0xfff;
            rec.flags.c = (u >> 13) & 1;
            rec.flags.e = (u >> 14) & 0xfff;

            for (int k = 0; k < 3; k++) {
                rec.pieces[k].f0 = *(int*)&unit->weapons[k];
                rec.pieces[k].f4 = unit->weapons[k].aimCob.nAimCobDone;
                rec.pieces[k].f8 = unit->weapons[k].weapon.get()->index;
                rec.pieces[k].fc = unit->weapons[k].muzzleAimFromDeltaZ;
                rec.pieces[k].f10 = unit->weapons[k].reloadTimer;
                rec.pieces[k].f12 = unit->weapons[k].fireHeading;
                rec.pieces[k].f14 = unit->weapons[k].firePitch;
                rec.pieces[k].f16 = unit->weapons[k].stockpile;
                rec.pieces[k].fl.b0 = ((PieceBits*)&unit->weapons[k].flags)->b0;
                rec.pieces[k].fl.b1 = ((PieceBits*)&unit->weapons[k].flags)->b1;
                rec.pieces[k].fl.b23 = ((PieceBits*)&unit->weapons[k].flags)->b23;
                rec.pieces[k].fl.b4 = ((PieceBits*)&unit->weapons[k].flags)->b4;
            }

            file->OpenNumberedBox(count);
            file->WriteBox(&rec, 0xb8);
            count++;
        }
    }
    if (count > 0) {
        file->SetIntegerItem("Number of Units", count);
        file->SetIntegerItem("Version", 0x11);
    }
}

static inline int* Elem_00487af0(int* arr, int i)
{
    return arr + i;
}

// FUNCTION: 0x487af0
int __stdcall FindMissionUnit(char* name, Struct_00487af0* param_2, int value)
{
    int i = 0;

    if (value) {
        while (*Elem_00487af0(param_2->ids, i++) != value) {
            if (i >= g_game->net->unitCount)
                return 0;
        }
    }
    if (i >= g_game->net->unitCount)
        return 0;
    for (; i < g_game->net->unitCount; i++) {
        MissionUnit* e = &g_game->net->units[i];
        if (e->ident && _strcmpi(e->ident, name) == 0 && param_2->ids[i])
            return param_2->ids[i];
        if (e->name && _strcmpi(e->name, name) == 0 && param_2->ids[i])
            return param_2->ids[i];
    }
    return 0;
}
