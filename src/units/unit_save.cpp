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

#include "../mission/mission.h"

#pragma pack(pop)

#include "../files/files_api.h"

#include "../scripts/cob_script.h"

#include "../movement/unit_motion.h"

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
#include "../game/map_events.h"
#include "../../interfaces/map.h"
#include "../../interfaces/orders.h"
#include "../../interfaces/movement.h"
#include "../game/economy_events.h"
#include "../../interfaces/economy.h"
#include "../../interfaces/los_radar.h"
#include "../../interfaces/weapons.h"
#include "../../interfaces/services.h"
#include "../game/ai_events.h"
#include "../game/weapons_events.h"
#include "../game/los_radar_events.h"
#include "../game/targeting_events.h"
#include "../../interfaces/targeting.h"
#include "../game/movement_events.h"

// FUNCTION: 0x4010b0
static void SaveUnitAccounts(Unit* info, HapiBank* file)
{
    char name[32];
    EconAccount account;
    g_seams.economy->UnitAccount(EconHandleOf(info), &account);
    sprintf(name, "u%04xacc", info->id);
    file->OpenNamedBox(name);
    file->SeekBox(0);
    file->WriteBox(&account.energy, 0x18);
    file->WriteBox(&account.metal, 0x18);
}

// FUNCTION: 0x401110
static void LoadUnitAccounts(Unit* info, HapiBank* file)
{
    char name[32];
    EconAccount account;
    g_seams.economy->UnitAccount(EconHandleOf(info), &account);
    sprintf(name, "u%04xacc", info->id);
    if (file->OpenNamedBox(name)) {
        file->SeekBox(0);
        file->ReadBox(&account.energy, 0x18);
        file->ReadBox(&account.metal, 0x18);
        EconPublishLoad(SCHED_UNITS, info->playerIndex, EconHandleOf(info), account);
    }
}

static void LoadUnitMotion(Unit* unit, HapiBank* file);
static void SaveUnitMotion(Unit* unit, HapiBank* file);

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
    LosRadarPublishSightLoad(SCHED_UNITS, unit, rec.f97, rec.bb0);
    *(int*)&unit->footprint = rec.f9b;
    unit->group = rec.f9f;
    AiPublishSetSquad(SCHED_UNITS, unit, rec.f9f);
    unit->speed_bits = rec.fa7;
    unit->lastDamageType = rec.bab;
    unit->healthPercent = rec.bac;
    unit->prevHealthPercent = rec.bad;
    unit->netDirtyFlags = rec.bae;
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

    LoadUnitAccounts(unit, file);
    if (rec.f27 != 0)
        LoadUnitMotion(unit, file);

    g_seams.orders->LoadOrders(unit, file, rec.f23);
    sprintf(script, "Script%i", i);
    file->OpenNamedBox(script);
    g_seams.scripts->LoadScriptState((CobScript*)unit->script.get(), file);

    for (int j = 0; j < 3; j++) {
        TargetingPublishLoad(SCHED_UNITS, unit, j, (short)rec.pieces[j].f0, (short)(rec.pieces[j].f0 >> 16));
        WeaponsSlotState state;
        state.aimDone = rec.pieces[j].f4;
        state.typeByte = rec.pieces[j].f8b;
        state.aimOffset = rec.pieces[j].fc;
        state.reload = rec.pieces[j].f10;
        state.heading = rec.pieces[j].f12;
        state.pitch = rec.pieces[j].f14;
        state.stockpile = rec.pieces[j].f16;
        state.flags = rec.pieces[j].fl.b0 | rec.pieces[j].fl.b1 << 1 | rec.pieces[j].fl.b23 << 2 | rec.pieces[j].fl.b4 << 4;
        g_seams.weapons->LoadState(unit, j, &state);
    }

    if (unit->cobStateFlags & 4)
        MapPublishUnit(SCHED_UNITS, MAP_RECLAIM_FOOTPRINT, unit);
    return unit;
    }
    return 0;
}

#pragma pack(push, 1)
struct UnitMotionSaveRecord {
    int velocity[3];
    int p2[3];
    int speed;
    short turn;
    int pathLockStamp;
    unsigned char mode : 2;
    unsigned char flag : 1;
};
#pragma pack(pop)

static void LoadUnitMotion(Unit* unit, HapiBank* file)
{
    char name[32];
    UnitMotionSaveRecord rec;
    sprintf(name, "u%04xmob", unit->id);
    file->OpenNamedBox(name);
    file->SeekBox(0);
    file->ReadBox(&rec, 0x23);
    MovementSavedState state;
    state.velocity[0] = rec.velocity[0];
    state.velocity[1] = rec.velocity[1];
    state.velocity[2] = rec.velocity[2];
    state.smoothedAcceleration[0] = rec.p2[0];
    state.smoothedAcceleration[1] = rec.p2[1];
    state.smoothedAcceleration[2] = rec.p2[2];
    state.speed = rec.speed;
    state.turn = rec.turn;
    state.mode = rec.mode;
    state.flag = rec.flag;
    MovementPublishLoad(SCHED_UNITS, unit, &state);
    MapPublishPathLockStamp(SCHED_UNITS, unit, rec.pathLockStamp);
}

// FUNCTION: 0x43dd70
static void SaveUnitMotion(Unit* unit, HapiBank* file)
{
    char name[32];
    MovementSavedState state;
    g_seams.movement->GetMotionState(unit, &state);
    UnitMotionSaveRecord hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.velocity[0] = state.velocity[0];
    hdr.velocity[1] = state.velocity[1];
    hdr.velocity[2] = state.velocity[2];
    hdr.p2[0] = state.smoothedAcceleration[0];
    hdr.p2[1] = state.smoothedAcceleration[1];
    hdr.p2[2] = state.smoothedAcceleration[2];
    hdr.speed = state.speed;
    hdr.turn = (short)state.turn;
    hdr.pathLockStamp = g_seams.map->PathLockStamp(unit);
    hdr.mode = state.mode;
    hdr.flag = state.flag;
    sprintf(name, "u%04xmob", unit->id);
    file->OpenNamedBox(name);
    file->SeekBox(0);
    file->WriteBox(&hdr, 0x23);
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
            char script[32];

            sprintf(script, "Script%i", count);
            file->OpenNamedBox(script);
            g_seams.scripts->SaveScriptState((CobScript*)unit->script.get(), file);

            int n = g_seams.orders->SaveOrders(unit, file);

            if (unit->motion != 0)
                SaveUnitMotion(unit, file);
            SaveUnitAccounts(unit, file);

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
            LosSightCache sight;
            g_seams.los_radar->SightCache(unit, &sight);
            rec.f97 = sight.cell;
            rec.f9b = *(int*)&unit->footprint;
            rec.bac = unit->healthPercent;
            rec.fa7 = unit->speed_bits;
            rec.bab = unit->lastDamageType;
            rec.bb0 = sight.frameIdx;
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
                rec.pieces[k].f0 = g_seams.targeting->SavedTargetPair(unit, k);
                WeaponsSlotState state;
                g_seams.weapons->SaveState(unit, k, &state);
                rec.pieces[k].f4 = state.aimDone;
                rec.pieces[k].f8 = state.typeByte;
                rec.pieces[k].fc = state.aimOffset;
                rec.pieces[k].f10 = state.reload;
                rec.pieces[k].f12 = state.heading;
                rec.pieces[k].f14 = state.pitch;
                rec.pieces[k].f16 = state.stockpile;
                rec.pieces[k].fl.b0 = state.flags & 1;
                rec.pieces[k].fl.b1 = state.flags >> 1 & 1;
                rec.pieces[k].fl.b23 = state.flags >> 2 & 3;
                rec.pieces[k].fl.b4 = state.flags >> 4 & 1;
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

// FUNCTION: 0x487af0
Unit* __stdcall FindMissionUnit(const char* name, const Ref<Unit>* units, int count, Unit* after)
{
    int i = 0;

    if (after) {
        while (units[i++].get() != after) {
            if (i >= count)
                return nullptr;
        }
    }
    if (i >= count)
        return nullptr;
    for (; i < count; i++) {
        MissionUnitInfo info;
        MissionUnitInfo* e = &info;
        g_seams.mission->GetMissionUnit(i, e);
        if (e->ident && _strcmpi(e->ident, name) == 0 && units[i].get())
            return units[i].get();
        if (e->name && _strcmpi(e->name, name) == 0 && units[i].get())
            return units[i].get();
    }
    return nullptr;
}
