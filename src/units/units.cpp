#include <stdio.h>
#include "unit_def.h"
#include "../state/block.h"
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#include "../map/mission.h"
#include "../util/tdf.h"

class MissionConditions;
class CobScript;
class UnitMotion;
class UnitResources;
struct SpotStateUnitView;
struct Object3do;

#pragma pack(push, 1)
#include "../map/cell.h"

#include "../util/vec3.h"

struct FeatureNextView {
    char unknown_0[0xf4];
    unsigned short next;
    char unknown_f6[0x100 - 0xf6];
};

#include "../network/player_info.h"

struct Unit;

#include "../network/player.h"


#include "cob_types.h"

union TypeFlags {
    unsigned int all;
    struct {
        unsigned int movOrder : 2;
        unsigned int fireOrder : 2;
        unsigned int initCloaked : 1;
        unsigned int b5 : 1;
        unsigned int b6 : 1;
        unsigned int b7 : 1;
        unsigned int b8 : 1;
        unsigned int b9 : 1;
        unsigned int b10 : 1;
        unsigned int b11 : 1;
        unsigned int b12 : 1;
        unsigned int b13 : 1;
        unsigned int b14 : 1;
        unsigned int b15 : 1;
        unsigned int hi : 16;
    } bits;
    struct {
        unsigned int lo : 18;
        unsigned int bit18 : 1;
        unsigned int mid : 5;
        unsigned int bit24 : 1;
        unsigned int top : 7;
    };
};

struct GroundHeightPos {
    Fixed x;
    Fixed y;
    Fixed z;
};



struct ClampedPos {
    Fixed x;
    Fixed y;
    Fixed z;
};

#include "../weapons/unit_weapon_slot.h"

#include "../map/spatial_bucket.h"





#include "../game/unit_resources.h"

#include "../util/angles.h"

#include "unit.h"

#include "../game/game.h"
#include "../quirks/quirks.h"
#pragma pack(pop)

// GLOBAL: 0x511de8
extern Game* g_game;

// FUNCTION: 0x485070
int __stdcall GetGroundHeight(GroundHeightPos* p)
{
    int x = p->x.whole;
    int z = p->z.whole;
    int xc = x >> 4;
    int zc = z >> 4;
    int xf = x & 15;
    int zf = z & 15;
    if (xc < 0 || xc + 1 >= g_game->width || zc < 0 || zc + 1 >= g_game->height)
        return -1;
    Cell* c = &g_game->cells[zc * g_game->width + xc];
    int a = c->height;
    int b = c[1].height;
    c += g_game->width;
    int e = c->height;
    int f = c[1].height;
    int r1 = a + (b - a) * xf / 16;
    return r1 + ((e + (f - e) * xf / 16) - r1) * zf / 16;
}

// FUNCTION: 0x4853b0
void __stdcall ClampPositionToMap(ClampedPos* p)
{
    if (p->x.value < 0)
        p->x.value = 0;
    else if (p->x.value >= MakeFixed(g_game->baseX).value)
        p->x = MakeFixed(g_game->baseX - 1);
    if (p->z.value < 0)
        p->z.value = 0;
    else if (p->z.value >= MakeFixed(g_game->baseY).value)
        p->z = MakeFixed(g_game->baseY - 1);
}

// FUNCTION: 0x485420
void __stdcall ShareMapInfo(unsigned char from, unsigned char to)
{
    unsigned short fromBit = 1 << from;
    unsigned short toBit = 1 << to;
    int n = g_game->width * g_game->height / 4;
    for (int i = 0; i < n; i++) {
        unsigned short* p = &g_game->visibilityMask[i];
        if (*p & fromBit)
            *p |= toBit;
    }
}

#include <iterator>
#include "../../interfaces/seams.h"
#include <xutility>

#define _ALGORITHM_

template<class _RI, class _Ty, class _Pr> void __stdcall QuickSort(_RI _F, _RI _L, _Pr _P, _Ty *);
template<class _RI, class _Ty, class _Pr> _RI __stdcall Partition(_RI _F, _RI _L, _Ty _Piv, _Pr _P);
template<class _RI, class _Ty, class _Pr> void __stdcall InsertShift(_RI _L, _Ty _V, _Pr _P);

namespace std {
const int _CHUNK_SIZE = 7;
const int _SORT_MAX = 16;

template<class _Ty, class _Pr> inline _Ty __stdcall _Median(_Ty _X, _Ty _Y, _Ty _Z, _Pr _P)
    {if (_P(_X, _Y))
        return (_P(_Y, _Z) ? _Y : _P(_X, _Z) ? _Z : _X);
    else
        return (_P(_X, _Z) ? _X : _P(_Y, _Z) ? _Z : _Y); }

template<class _FI1, class _FI2> inline void __stdcall iter_swap(_FI1 _X, _FI2 _Y)
    {_Iter_swap(_X, _Y, _Val_type(_X)); }
template<class _FI1, class _FI2, class _Ty> inline void __stdcall _Iter_swap(_FI1 _X, _FI2 _Y, _Ty *)
    {_Ty _Tmp = *_X;
    *_X = *_Y, *_Y = _Tmp; }

template<class _RI, class _Pr> inline void __stdcall sort(_RI _F, _RI _L, _Pr _P)
    {_Sort_0(_F, _L, _P, _Val_type(_F)); }
template<class _RI, class _Ty, class _Pr> inline void __stdcall _Sort_0(_RI _F, _RI _L, _Pr _P, _Ty *)
    {if (_L - _F <= _SORT_MAX)
        _Insertion_sort(_F, _L, _P);
    else
        {QuickSort(_F, _L, _P, (_Ty *)0);
        _Insertion_sort(_F, _F + _SORT_MAX, _P);
        for (_F += _SORT_MAX; _F != _L; ++_F)
            InsertShift(_F, _Ty(*_F), _P); }}
template<class _RI, class _Pr> inline void __stdcall _Insertion_sort(_RI _F, _RI _L, _Pr _P)
    {_Insertion_sort_1(_F, _L, _P, _Val_type(_F)); }
template<class _RI, class _Ty, class _Pr> inline void __stdcall _Insertion_sort_1(_RI _F, _RI _L, _Pr _P, _Ty *)
    {if (_F != _L)
        for (_RI _M = _F; ++_M != _L; )
            {_Ty _V = *_M;
            if (!_P(_V, *_F))
                InsertShift(_M, _V, _P);
            else
                {copy_backward(_F, _M, _M + 1);
                *_F = _V; }}}
}

template<class _RI, class _Ty, class _Pr> void __stdcall InsertShift(_RI _L, _Ty _V, _Pr _P)
    {for (_RI _M = _L; _P(_V, *--_M); _L = _M)
        *_L = *_M;
    *_L = _V; }

template<class _RI, class _Ty, class _Pr> _RI __stdcall Partition(_RI _F, _RI _L, _Ty _Piv, _Pr _P)
    {for (; ; ++_F)
        {for (; _P(*_F, _Piv); ++_F)
            ;
        for (; _P(_Piv, *--_L); )
            ;
        if (_L <= _F)
            return (_F);
        std::iter_swap(_F, _L); }}

template<class _RI, class _Ty, class _Pr> void __stdcall QuickSort(_RI _F, _RI _L, _Pr _P, _Ty *)
    {for (; std::_SORT_MAX < _L - _F; )
        {_RI _M = Partition(_F, _L, std::_Median(_Ty(*_F),
            _Ty(*(_F + (_L - _F) / 2)), _Ty(*(_L - 1)), _P), _P);
        if (_L - _M <= _M - _F)
            QuickSort(_M, _L, _P, std::_Val_type(_F)), _L = _M;
        else
            QuickSort(_F, _M, _P, std::_Val_type(_F)), _F = _M; }}

#include <algorithm>

#include "units.h"

// FUNCTION: 0x4854a0
void __stdcall AllocateUnitMemory(void)
{
    g_game->focusUnitId = 0;
    g_game->autoFollowFlags_14373 &= 0xfffffffd;
    g_game->unitsPerPlayer = g_game->maxUnitsSetting;
    g_game->poolCount = (unsigned short)(g_game->maxUnitsSetting * 10 + 1);

    unsigned char* pool = (unsigned char*)SimAllocTagged("UNIT MEMORY", g_game->poolCount * 0x118);
    g_game->units = (Unit*)pool;
    memset(pool, 0, g_game->poolCount * 0x118);

    unsigned int ten = g_game->maxUnitsSetting * 10;
    (*(void **)&g_game->list_1435f) = SimAllocTagged("HOT UNITS", ten * 2);
    g_game->hotRadar = SimAllocTagged("HOT RADAR UNITS", ten * 10);
    g_game->unitsEnd = (Unit*)((unsigned char *)g_game->units.get() + g_game->poolCount * 0x118 - 0x118);

    unsigned short n;
    for (n = 0; n < g_game->poolCount; n++) {
        *(unsigned short*)(pool + n * 0x118 + 0xa8) = n;
        *(Ref<UnitDef>*)(pool + n * 0x118 + 0x92) = g_game->defs;
    }

    Player* v[10];
    int k;
    for (k = 0; k < 10; k++)
        v[k] = (Player*)((unsigned char*)g_game->players + k * 0x14b);

    std::sort(v, v + 10, ComparePlayers);

    pool[0xff] = 0xff;
    *(unsigned int*)(pool + 0x96) = 0;
    int i;
    for (i = 0; i < 10; i++) {
        Player* item = v[i];
        int c = g_game->maxUnitsSetting * i + 1;
        item->unitsBegin = (Unit*)(pool + c * 0x118);
        item->unitsEnd = (Unit*)((unsigned char*)item->unitsBegin.get() + g_game->maxUnitsSetting * 0x118 - 0x118);
        *(unsigned short*)((char*)item + 0x6f) =
            *(unsigned short*)((unsigned char*)item->unitsBegin.get() + 0xa8);
        *(unsigned short*)((char*)item + 0x71) =
            *(unsigned short*)((unsigned char*)item->unitsEnd.get() + 0xa8);
        for (unsigned char* q = (unsigned char*)item->unitsBegin.get();
             q <= (unsigned char*)item->unitsEnd.get(); q += 0x118) {
            ((Unit*)q)->player = item;
            q[0xff] = *(unsigned char*)((char*)item + 0x146);
            *(unsigned int*)(q + 0xac) = 0xffffffffu;
        }
    }
}

// FUNCTION: 0x485940
int __stdcall ComparePlayers(Player* a, Player* b)
{
    if (g_seams.map->GetGameType(g_game->net) == 3)
        return a->id < b->id;
    return a < b;
}

#include "../graphics/object_pool.h"

// FUNCTION: 0x485980
void FreeUnitMemory(void)
{
    Unit* u = g_game->units;
    Unit* end = g_game->unitsEnd;
    if (u != 0) {
        for (; u <= end; u = (Unit*)((char*)u + 0x118)) {
            if (u->unitDefIndex != 0)
                KillUnit(u, 8);
        }
    }
    if (g_game->hotRadar != 0)
        GameFreeThunk(g_game->hotRadar);
    g_game->hotRadar = 0;
    if ((void *)g_game->list_1435f != 0)
        GameFreeThunk((void *)g_game->list_1435f);
    (*(void **)&g_game->list_1435f) = 0;
    if (g_game->units != 0)
        GameFreeThunk(g_game->units);
    g_game->units = 0;
}


#include "../util/math.h"

// FUNCTION: 0x485a40
void __stdcall InitUnitFromType(Unit* unit, Vec3 pos, int param_5)
{
    unit->def = &g_game->defs.get()[(unsigned short)unit->unitDefIndex];
    unit->alive = 1;
    unit->immobile = (unit->def->mobile == 0);
    unit->killPending = 0;
    unit->footprint = unit->def->origin;
    unit->armed = ((TypeFlags*)&unit->def->flags1)->bits.hi;
    unit->zBufferBit0 = ((TypeFlags*)&unit->def->flags1)->bits.b7;
    unit->cargoSelectable = ((TypeFlags*)&unit->def->flags1)->bits.b9;

    if (param_5) {
        unit->buildLeft = 0;
        unit->health = unit->def->hp;
    } else {
        unit->buildLeft = 1.0f;
        unit->unfinishedInitZero = 0;
        unit->health = 0;
    }

    unit->bit0 = 0;
    unit->bit1 = 0;
    unit->bit2 = 0;
    unit->bit3 = 0;
    unit->mode = 1;
    unit->unknown_110_2 = 0;
    unit->selected = 0;
    unit->selectable = 1;
    unit->unknown_110_10 = 0;
    unit->cloakOn = 0;
    unit->moved = 1;
    unit->carriedNoPiece = 0;
    unit->healthPercent = 0;
    unit->prevHealthPercent = 0;
    unit->activateFlags = 0;
    unit->workTime = 0;
    unit->angles.pitch = 0;
    unit->pos = pos;

    Point16 off = unit->footprint;
    Point16 screen;
    screen.x = (short)((pos.x - off.x * 0x80000 + 0x80000) >> 20);
    screen.y = (short)((pos.z - off.y * 0x80000 + 0x80000) >> 20);
    unit->cell = screen;

    unit->angles.heading = (short)(RandomInt(unit->def->buildangleUnsigned)
                                    + (0x8000 - unit->def->buildangleUnsigned / 2));
    unit->angles.bank = 0;
    unit->losCacheCellX = 0;
    unit->losCacheCellZ = 0;
    unit->recentlyDamagedTimer = 0;
    unit->postTransferHoldoff = 0;
    unit->ownerIsLocal = (unit->player->index == g_game->playerIndex);
    unit->unknown_110_8 = 0;

    for (int i = 0; i < 3; i++) {
        g_seams.targeting->ResetWeaponTarget((Unit*)unit, i);
        g_seams.weapons->ReleaseWeapons((Unit*)unit, 0, i);
    }

    unit->netDirtyFlags = 0;
    unit->killCount = 0;
    unit->attacker = 0;
    unit->lastAttackerSlot = 0xa;
    unit->moveOrder = ((TypeFlags*)&unit->def->flags1)->bits.movOrder;
    unit->fireOrder = ((TypeFlags*)&unit->def->flags1)->bits.fireOrder;
    unit->cloakOn = ((TypeFlags*)&unit->def->flags1)->bits.initCloaked;
    unit->f26_27 = 0;
    if (unit->def->buildMenuPageCount > 1) {
        unit->f22_23 = 3;
        unit->f24_25 = 0;
    } else {
        unit->f22_23 = 0;
        unit->f24_25 = 0;
    }

    unit->losSightFrameIdx = 0;
    g_seams.economy->Reset(&unit->resourceSlot, unit->playerIndex);
    unit->transportPiece = 0xff;
    unit->hoverBobPhase = (short)RandomInt(0x10000);
    g_seams.ai->SetUnitSquad(unit, 0);
}

struct SpotStateUnitView {
    char unknown_0[8];
    int dirty;
    void* unit;
    int drawFrame;
};

#include "cob_script.h"

#include "../sound/sound.h"
struct BmpFileHeader;
struct BmpInfo;
struct BmpInfoHeader;


#include "unit_script.h"

void* __cdecl operator new(size_t size);
SpotStateUnitView* __stdcall CreatePlayerObjectState(Object3do* obj, ScriptTable* data, int player);
#include "../graphics/model_render.h"

// FUNCTION: 0x485d40
void __stdcall InitUnitScript(Unit* self)
{
    Object3do* obj = ((Object3do **)g_game->types.get())[self->unitDefIndex];
    if (self->def->data) {
        self->script = new (kSim) UnitScript;
        ((CobScript*)self->script.get())->SetCob((ScriptTable*)self->def->data);
        self->state = (SpotState*)CreatePlayerObjectState(obj, (ScriptTable*)self->def->data, (int)self);
        ((UnitScript*)self->script.get())->SetObjectState((int)self->state.get());
        ((CobScript*)self->script.get())->StartScript("Create", 0, 1);
    } else {
        self->script = 0;
        self->state = (SpotState*)CreateObjectState(obj);
        ((SpotStateUnitView*)self->state.get())->unit = self;
    }
    ((SpotStateUnitView*)self->state.get())->drawFrame = 0;
}

// FUNCTION: 0x485e30 ??_GUnitScript@@UAEPAXI@Z
static UnitScript* s_object;
UnitScript* EmitUnitScriptDeletingDtor() { return new UnitScript; }

#include "../orders/unit_motion.h"

// FUNCTION: 0x485e50
void __stdcall CreateUnitMotion(Unit* unit)
{
    unit->motion = new (kSim) UnitMotion(unit);
    unit->angles.heading = unit->def->buildangleUnsigned;
}

extern void* g_weaponAimCobVtable[];

// FUNCTION: 0x485e90
void __stdcall InitUnit(int unitType, Vec3 pos, int param_5, Unit* unit)
{
    UnitDef* type = &g_game->defs.get()[(unsigned short)unitType];
    if (unit) {
        for (int i = 0; i < 3; i++)
            unit->weapons[i].aimCob.ppVtbl = g_weaponAimCobVtable;
    }
    unit->unitDefIndex = (short)unitType;
    InitUnitFromType(unit, pos, param_5);
    InitUnitScript(unit);
    g_seams.weapons->InitUnitWeaponSlots(unit);
    g_seams.economy->UpdateMetalExtraction((Unit*)unit);
    if (type->mobile == 1) {
        unit->motion = new (kSim) UnitMotion(unit);
        unit->angles.heading = unit->def->buildangleUnsigned;
    }
}

#include "../game/mission_conditions.h"
#include "../state/sim_globals.h"

#include "unit_position.h"
#include "../map/terrain.h"
#include "../network/net_game.h"

static inline void __stdcall InitSpawnedUnit(unsigned short unitType, Vec3 pos,
                                               int param_5, Unit* unit)
{
    UnitDef* type = &g_game->defs.get()[unitType];
    if (unit) {
        for (int i = 0; i < 3; i++)
            unit->weapons[i].aimCob.ppVtbl = g_weaponAimCobVtable;
    }
    unit->unitDefIndex = unitType;
    InitUnitFromType(unit, pos, param_5);
    InitUnitScript(unit);
    g_seams.weapons->InitUnitWeaponSlots(unit);
    g_seams.economy->UpdateMetalExtraction((Unit*)unit);
    if (type->mobile == 1) {
        unit->motion = new (kSim) UnitMotion(unit);
        unit->angles.heading = unit->def->buildangleUnsigned;
    }
}

// FUNCTION: 0x485f50
Unit* __stdcall CreateUnit(unsigned char player, unsigned short typeId, Vec3 pos,
                                      int param_5, int mode, unsigned short id)
{
    int off = player * 0x14b;
    Player* pl = (Player*)((char*)g_game + off + 0x1b63);
    if (typeId == 0)
        return 0;
    UnitDef* type = &g_game->defs.get()[typeId];
    if (!(type->flags1 & 0x800000))
        return 0;
    if (type->maxThisUnit != -1) {
        int count = 0;
        for (Unit* u = pl->unitsBegin; u <= pl->unitsEnd; u++) {
            if (u->unitDefIndex == typeId)
                count++;
        }
        if (count >= type->maxThisUnit)
            return 0;
    }
    Unit* unit;
    for (unit = pl->unitsBegin; unit <= pl->unitsEnd; unit++) {
        if (id != 0) {
            unit = &g_game->units[id];
            if (unit < pl->unitsBegin || unit > pl->unitsEnd)
                return 0;
        }
        if (unit->unitDefIndex == 0)
            goto found;
        if (id != 0)
            return 0;
    }
    return 0;
found:
    InitSpawnedUnit(typeId, pos, param_5, unit);
    unit->mode = mode;
    UpdateUnitHeight(unit);
    g_seams.map->AddUnitToMap(unit);
    g_seams.network->SendNewUnit(unit);
    if (param_5) {
        if (type->mobile == 0)
            g_seams.network->SendBuilderLink(unit, unit);
        if (((TypeFlags*)&unit->def->flags1)->bit18)
            ((Unit*)unit)->SetStateBits(1, 1);
        if (((TypeFlags*)&unit->def->flags1)->bit24) {
            unit->lastDamageType = 7;
            unit->killPending = 1;
        }
    }
    g_seams.los_radar->RevealNewUnit(unit);
    g_game->players[player].unitCount++;
    g_game->players[player].unitsCreated++;
    g_seams.players->NotifyUnitCreated(g_game->list_391ed, 0, unit);
    return unit;
}

#pragma pack(push, 1)
struct UnitSpawnPacket {
    unsigned char player;
    unsigned short type;
    unsigned short id;
    Vec3 pos;
};
#pragma pack(pop)

// FUNCTION: 0x4861d0
Unit* __stdcall CreateUnitFromPacket(unsigned char player, UnitSpawnPacket* spawn)
{
    Player* pl = &g_game->players[player];
    Unit* unit;
    if (spawn->id == 0) {
        unit = 0;
    } else {
        unit = &g_game->units[spawn->id];
    }
    if (pl->unitsBegin == 0) {
        return 0;
    }
    if (unit->unitDefIndex != 0) {
        KillUnit(unit, 0);
    }
    InitSpawnedUnit(spawn->type, spawn->pos, 0, unit);
    UpdateUnitHeight(unit);
    AddUnitToMap(unit);
    g_seams.los_radar->RevealNewUnit(unit);
    g_game->players[player].unitCount++;
    g_game->players[player].unitsCreated++;
    g_game->list_391ed->NotifyUnitCreated(unit);
    return unit;
}

struct PlacedFeatureView {
    char unknown_0[0x18];
    int velY;
    int velZ;
};

void __stdcall EmitSmoke(GroundHeightPos* pos, int a, int b, int c);

// FUNCTION: 0x486360
void __stdcall CreateUnitCorpse(Unit* unit, int depth, int flag)
{
    unsigned short id = unit->def->corpseUnsigned;
    for (; depth > 1; depth--) {
        if (id >= 0xfffb) {
            return;
        }
        id = ((FeatureNextView *)g_game->features)[id].next;
    }
    if (id < 0xfffb) {
        void* target = g_seams.los_radar->GetMapCell(unit->cell.x, unit->cell.y);
        if (target != 0) {
            GroundHeightPos* pos = (GroundHeightPos*)&unit->pos;
            if (GetGroundHeight(pos) <= g_game->seaLevel) {
                PlacedFeatureView* r = (PlacedFeatureView*)g_seams.features->PlaceFeature((Cell*)target, id, (Vec3*)pos, (Rot16*)&unit->angles, unit->playerIndex);
                if (r != 0) {
                    if (!(unit->def->flags1 & 0x1000000)) {
                        r->velY = -11468;
                        r->velZ = 0;
                    }
                    flag = 0;
                }
            } else {
                g_seams.features->PlaceFeature((Cell*)target, id, (Vec3*)pos, (Rot16*)&unit->angles, unit->playerIndex);
            }
            if (flag) {
                EmitSmoke(pos, 0xf, 900, 9);
            }
        }
    }
}

// FUNCTION: 0x486460
int __stdcall IsUnitCommander(Unit* unit)
{
    return _strcmpi(g_game->sideDefs[unit->player->info->side].commander, unit->def->unitname) == 0;
}

#include "../network/wire_packets.h"
#include "../game/game_state.h"

// FUNCTION: 0x4864b0
void __stdcall KillUnit(Unit* unit, int param_2)
{
    if ((unit->flags & 0x10000000) != 0) {
        int same = _strcmpi(g_game->sideDefs[unit->player->info->side].commander,
                            unit->def->unitname) == 0;
        if (same) {
            unit->player->flags &= 0xfffe;
        }
        int amount;
        int flag;
        if (param_2 == 7) {
            flag = 1;
            amount = 0;
        } else if (param_2 == 4 || param_2 == 5 || param_2 == 9 || unit->health > 0) {
            amount = 0;
            flag = 0;
        } else {
            amount = ((int)(unit->health * -100 / unit->def->maxHealth) + unit->prevHealthPercent) / 2;
            if (amount < 1)
                amount = 1;
            if (amount > 100)
                amount = 100;
            ((CobScript*)unit->script.get())->QueryScript("Killed", &amount, &flag, 0, 0);
        }
        if (unit->buildLeft != 0.0f) {
            flag = 0;
        }
        UnitKilledPacket cmd;
        cmd.unitId = unit->id;
        cmd.cobKilledSeverity = amount;
        cmd.type = 0xc;
        cmd.wreckageLevel = flag;
        cmd.killType = param_2;
        cmd.attackerPlayerNetId = GetSlotDpid(unit->lastAttackerSlot);
        if (unit->attacker == 0)
            cmd.attackerUnitId = 0;
        else
            cmd.attackerUnitId = unit->attacker->id;
        if (unit->player->active != 0 &&
            (unit->player->type == 1 || unit->player->type == 2)) {
            g_seams.network->SendUnitKilled(unit, amount, flag, param_2);
        }
        ApplyUnitDeath(&cmd, 1);
        if (same && g_game->commanderDeath != 0 && unit->player->active != 0 &&
            (unit->player->type == 1 || unit->player->type == 2)) {
            PopUntilNamedLayout(1);
            g_seams.players->KillPlayerUnits(unit->playerIndex);
        }
    }
}

#include "../../link/globals.h"

void __stdcall RemoveSpeechOfUnit(void* unit);
void __stdcall AttachUnitToPiece(void* unit, void* builder, int a, int c);
#include "../ingame/game_dialogs.h"
void __stdcall AddMessage(char* text, int a, int b, int c);
#include "unit_targets.h"
void __stdcall FreeObjectState(void* state);
void __cdecl operator delete(void* p);
#include "../frontend/skirmish_menu.h"

// FUNCTION: 0x4866d0
void __stdcall ApplyUnitDeath(UnitKilledPacket* cmd, int local)
{
    Unit* unit;
    if (cmd->unitId == 0)
        unit = 0;
    else
        unit = &g_game->units[cmd->unitId];
    if (g_quirks.ignoreNullDeathRecord && unit == 0)
        return;
    if ((unit->flags & 0x10000000) == 0)
        return;

    if (unit->player->index == g_game->playerIndex)
        g_seams.los_radar->AddEyeball((Vec3*)&unit->pos, unit->def->range, unit->def->modelMaxYHigh, 60);
    Unit* parent;
    if (cmd->attackerUnitId == 0)
        parent = 0;
    else
        parent = &g_game->units[cmd->attackerUnitId];
    unit->attacker = parent;
    unit->lastAttackerSlot = FindSlotByDpid(cmd->attackerPlayerNetId);
    g_seams.players->NotifyUnitDied(g_game->list_391ed, 0, unit);
    g_seams.orders->DeleteOrders(unit, 1);
    RemoveSpeechOfUnit(unit);
    g_seams.ai->SetUnitSquad(unit, -1);
    g_seams.weapons->RemoveUnitProjectiles(unit);
    if (unit->carrier != 0)
        AttachUnitToPiece(unit, 0, -1, 1);
    while (unit->cargo != 0) {
        unsigned char depth = cmd->killType != 3 ? 6 : 3;
        g_seams.weapons->DamageUnit(unit->attacker, unit->cargo, 30000, depth, 0);
        AttachUnitToPiece(unit->cargo, 0, -1, 1);
    }
    g_seams.map->ClearFootprintAndUnlink(unit);
    if ((g_game->mapFlags & 2) == 2)
        g_seams.los_radar->RemoveUnitLineOfSight(unit);
    if (local == 0 && cmd->cobKilledSeverity > 0)
        ((CobScript*)unit->script.get())->StartScriptWithArgs(g_killedScriptName, 0, 1, 1, cmd->cobKilledSeverity, 0, 0, 0);

    int credited = 0;
    switch (cmd->killType) {
    case 5:
        if (unit->lastAttackerSlot == 10 || unit->lastAttackerSlot == unit->playerIndex)
            break;
    case 1:
    case 6:
        if (unit->player != 0) {
            unit->player->losses++;
            if (unit->lastAttackerSlot != 10 && unit->buildLeft == 0.0f && unit->playerIndex != unit->lastAttackerSlot)
                g_game->players[unit->lastAttackerSlot].kills++;
            int same = _strcmpi(g_game->sideDefs[unit->player->info->side].commander,
                                unit->def->unitname) == 0;
            if (same) {
                if (unit->lastAttackerSlot != 10)
                    g_game->players[unit->lastAttackerSlot].commanderKills++;
                unit->player->commanderLosses++;
            }
            if (unit->attacker != 0 && unit->buildLeft == 0.0f && unit->playerIndex != unit->lastAttackerSlot)
                unit->attacker->killCount++;
            if (unit->lastAttackerSlot == g_game->localPlayer)
                AddCdActivitySample(5);
            credited = 1;
        }
        break;
    case 3:
        if (unit->player != 0 && g_game->players[g_game->localPlayer].shareVision[unit->player->index] == 0) {
            unit->player->losses++;
            int same = _strcmpi(g_game->sideDefs[unit->player->info->side].commander,
                                unit->def->unitname) == 0;
            if (same)
                unit->player->commanderLosses++;
            credited = 1;
        }
        break;
    }

    if (credited && unit->lastAttackerSlot != 10) {
        Player* rec = &g_game->players[unit->lastAttackerSlot];
        if (rec->active != 0 && (rec->type == 1 || rec->type == 2 || rec->type == 3)
            && rec->index != 10
            && (g_seams.map->GetGameType(g_game->net) == 3 || g_seams.map->GetGameType(g_game->net) == 2)
            && rec->rank > 0) {
            int rank = rec->rank;
            int best = rank;
            int mine = g_game->commanderDeath == 2 ? rec->commanderKills : rec->kills;
            int i = 10;
            Player* p = g_game->players;
            do {
                if (p->type != 0) {
                    bool hid = p->info->bit6;
                    if (!hid) {
                        int ahead;
                        if (g_game->commanderDeath == 2)
                            ahead = mine > p->commanderKills;
                        else
                            ahead = mine > p->kills;
                        if (ahead) {
                            if (p->rank < best)
                                best = p->rank;
                        }
                    }
                }
                p++;
                i--;
            } while (i != 0);
            if (best < rank) {
                i = 10;
                p = g_game->players;
                do {
                    if (p->rank >= best && p->rank < rec->rank)
                        p->rank = p->rank + 1;
                    p++;
                    i--;
                } while (i != 0);
                rec->rank = best;
                if (best == 0) {
                    char text[100];
                    sprintf(text, Translate(g_takenLeadFormat), rec->name,
                            g_game->commanderDeath == 2 ? rec->commanderKills : rec->kills);
                    AddMessage(text, 2, 0, 10);
                }
            }
        }
        if (g_game->bits_37f06_1.b7)
            FlashScorePanelKillLoss(unit->lastAttackerSlot, unit->player->index);
    }

    if (cmd->killType == 5 && unit->attacker != 0) {
        Ref<Unit>* par = &unit->attacker;
        float health = 1.0f - unit->buildLeft;
        float f = health;
        f *= unit->def->metalCost;
        if ((*par)->resourceSlot.player->active != 0 && (*par)->resourceSlot.player->type == 2) {
            switch (g_game->difficulty) {
            case 0:
                (*par)->resourceSlot.metalMake = (*par)->resourceSlot.metalMake - f * -0.5;
                break;
            case 1:
                (*par)->resourceSlot.metalMake = (*par)->resourceSlot.metalMake - f * -0.7;
                break;
            default:
                (*par)->resourceSlot.metalMake += f;
            }
        } else {
            (*par)->resourceSlot.metalMake += f;
        }
    }
    if (cmd->cobKilledSeverity > 0 && unit->buildLeft == 0.0f)
        g_seams.weapons->DetonateUnitWeapon(unit, cmd->killType == 3);
    if (cmd->wreckageLevel > 0) {
        int flag = cmd->killType != 7;
        CreateUnitCorpse(unit, cmd->wreckageLevel, flag);
    }

    ClearUnitRefs(unit);
    if (unit->script != 0) {
        delete (CobScript*)unit->script.get();
        unit->script = 0;
    }
    if (unit->state != 0) {
        FreeObjectState(unit->state);
        unit->state = 0;
    }
    UnitMotion* head = unit->motion;
    if (head != 0) {
        g_seams.orders->DestroyMotionObject(head);
        operator delete(head);
        unit->motion = 0;
    }
    unit->unitDefIndex = 0;
    unit->flags &= ~0x10000000;
    unit->def = g_game->defs.get();
    unit->flags &= ~0x30;
    unit->player->unitCount--;
    if (unit->player->unitCount == 0) {
        if (g_seams.map->GetGameType(g_game->net) == 3)
            AnnouncePlayerLeft(unit->player->id);
        if (g_seams.map->GetGameType(g_game->net) == 2)
            AnnounceForcesDestroyed(unit->player);
    }
}

// FUNCTION: 0x486e80
void __stdcall KillUnitsOfType(short id)
{
    Unit* u = g_game->units;
    Unit* end = g_game->unitsEnd;
    if (id != 0 && u != 0) {
        for (; u <= end; u = (Unit*)((char*)u + 0x118)) {
            if ((short)u->unitDefIndex == id) {
                KillUnit(u, 8);
            }
        }
    }
}

// FUNCTION: 0x486ed0
void KillAllUnits(void)
{
    Unit* u = g_game->units;
    Unit* end = g_game->unitsEnd;
    if (u != 0) {
        for (; u <= end; u = (Unit*)((char*)u + 0x118)) {
            if (u->unitDefIndex != 0) {
                KillUnit(u, 8);
            }
        }
    }
}

// FUNCTION: 0x486f10
void __stdcall KillPlayerUnits(unsigned char player)
{
    Player* p = &g_game->players[player];
    if (p != 0) {
        if (p->unitCount != 0) {
            Unit* u = g_game->players[player].unitsBegin;
            Unit* last = g_game->players[player].unitsEnd;
            if (u != 0) {
                for (; u <= last; u++) {
                    unsigned int flags = u->flags;
                    if (flags & 0x10000000) {
                        if (!(flags & 0x4000)) {
                            Player* d = u->player;
                            if (d->active != 0 &&
                                (d->type == 1 || d->type == 2)) {
                                g_seams.weapons->DamageUnit(u, u, 0x7530, 3, 0);
                            } else {
                                g_seams.weapons->DetonateUnitWeapon(u, 1);
                                u->flags |= 0x4000;
                                g_seams.units->KillUnit(u, 3);
                            }
                        }
                    }
                }
            }
        }
    }
}
