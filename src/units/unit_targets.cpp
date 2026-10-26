#include "win_types.h"
#include "../../interfaces/ref.h"
#include "../state/block.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

#include "cob_script.h"
#include "../util/angles.h"
#include "../quirks/quirks.h"

class UnitRef;

class Listener_004896f0 {
public:
    virtual void Notify(int code);
};

#pragma pack(push, 1)
struct Point_004898b0 {
    short a;
    short b;
};

struct Target_0048a1e0 {
    char unknown_0[0x68];
    int radius;
    char unknown_6c[0x111 - 0x6c];
    unsigned int field_111;
};

struct UnitType {
    char unknown_0[0x152];
    int count;
    Ref<short> ids;
    char unknown_15a[0x186 - 0x15a];
    float energyCost;
    float metalCost;
    char unknown_18e[0x192 - 0x18e];
    int sight;
    char unknown_196[0x19e - 0x196];
    int field_19e;
    char unknown_1a2[0x1aa - 0x1a2];
    int f1aa;
    char unknown_1ae[0x1ba - 0x1ae];
    unsigned short field_1ba;
    char unknown_1bc[0x1ea - 0x1bc];
    int field_1ea;
    char unknown_1ee[0x1fa - 0x1ee];
    unsigned int maxhp;
    char unknown_1fe[0x22f - 0x1fe];
    unsigned char mobile;
    char unknown_230[0x241 - 0x230];
    unsigned int f241;
};

struct Vec3_0048a1e0 {
    int x;
    int y;
    int z;
};

struct Body_0048a1e0 {
    char unknown_0[8];
    Vec3_0048a1e0 offset;
    char unknown_14[0x20 - 0x14];
    int sight;
    char unknown_24[0x2a - 0x24];
    int age;
};

#include "../network/player.h"

#include "unit.h"

struct MapInfo_0048a490;

#include "../game/game.h"
#pragma pack(pop)

#include "../../link/globals.h"

#include "../debug/debug_lib.h"
char* __stdcall Translate(const char* text);
#include "../graphics/display.h"

static inline float spd(int v)
{
    return v * 1.52587890625e-05f;
}

// FUNCTION: 0x489280
char* __stdcall MakePropList(UnitType* obj)
{
    char* buf = (char*)GameAllocIgnoreTag("PropList", 0xc0);
    memset(buf, 0, 0xc0);
    char* p = buf;

    sprintf(p, "\n");
    p += strlen(p) + 1;

    sprintf(p, "%d", (int)obj->energyCost);
    p += strlen(p) + 1;

    sprintf(p, "%d", (int)obj->metalCost);
    p += strlen(p) + 1;

    sprintf(p, "%d", obj->field_1ea);
    p += strlen(p) + 1;

    sprintf(p, "\n");
    p += strlen(p) + 1;

    if (obj->mobile) {
        sprintf(p, "%.1f %s ", (double)GetTickRate() * spd(obj->sight) * 0.4,
                Translate("m/s"));
        p += strlen(p) + 1;

        sprintf(p, "%.2f %s", (double)GetTickRate() * spd(obj->field_19e) * 0.4,
                Translate("m/s/s"));
        p += strlen(p) + 1;

        sprintf(p, "%.0f %s",
                (double)GetTickRate() * obj->field_1ba * 0.0054931640625,
                Translate("deg/s"));
    } else {
        sprintf(p, "%s", Translate("N/A"));
        p += strlen(p) + 1;

        sprintf(p, "%s", Translate("N/A"));
        p += strlen(p) + 1;

        sprintf(p, "%s", Translate("N/A"));
    }

    return buf;
}

// FUNCTION: 0x4894f0
int __stdcall UnitCanBuild(Unit* unit, short id)
{
    for (int i = 0; i < ((UnitType*)unit->def.get())->count; i++) {
        if (((UnitType*)unit->def.get())->ids[i] == id)
            return 1;
    }
    return 0;
}

#include "unit_ref.h"

// FUNCTION: 0x489540
void UnitRef::LinkToUnit(Unit* o)
{
    if (o != 0 && o->unitDefIndex != 0) {
        owner = o;
        next = (UnitRef*)o->head.get();
        o->head = (PathOrderAttach*)this;
        return;
    }
    owner = 0;
    next = 0;
}

// FUNCTION: 0x489580
void UnitRef::UnlinkFromUnit()
{
    if (owner != 0) {
        Ref<UnitRef>* link = (Ref<UnitRef>*)&owner->head;
        while (*link != this)
            link = &(*link)->next;
        *link = next;
        owner = 0;
        next = 0;
    }
}

#include "../orders/path_order_attach.h"

// FUNCTION: 0x4895c0
PathOrderAttach::PathOrderAttach(Unit* o, int v)
    : value((void*)v)
{
    if (o != 0 && o->unitDefIndex != 0) {
        owner = o;
        next = o->head;
        o->head = this;
        return;
    }
    owner = 0;
    next = 0;
}

// FUNCTION: 0x489600 ??_GPathOrderAttach@@UAEPAXI@Z

// FUNCTION: 0x489650
void UnitRef::Unlink()
{
    ((PathOrderAttach*)this)->PathOrderAttach::~PathOrderAttach();
}

inline PathOrderAttach::~PathOrderAttach()
{
    if (owner != 0) {
        Ref<PathOrderAttach>* pp = &owner->head;
        while (*pp != this)
            pp = &(*pp)->next;
        *pp = next;
        owner = 0;
        next = 0;
    }
}

// FUNCTION: 0x489690
void PathOrderAttach::SetUnit(Unit* o)
{
    if (owner != 0) {
        Ref<PathOrderAttach>* pp = &owner->head;
        while (*pp != this)
            pp = &(*pp)->next;
        *pp = next;
        owner = 0;
        next = 0;
    }
    if (o != 0 && o->unitDefIndex != 0) {
        owner = o;
        next = o->head;
        o->head = this;
        return;
    }
    owner = 0;
    next = 0;
}

// FUNCTION: 0x4896f0
void UnitRef::ClearRef(void)
{
    Unit* saved = owner;
    if (listener)
        listener->Notify(8);
    if (owner == saved && owner) {
        Ref<UnitRef>* pp = (Ref<UnitRef>*)&owner->head;
        while (*pp != this)
            pp = &(*pp)->next;
        *pp = next;
        owner = 0;
        next = 0;
    }
}

// FUNCTION: 0x489740
void __stdcall ClearUnitRefs(Unit* obj)
{
    for (UnitRef* n = (UnitRef*)obj->head.get(); n != 0; n = (UnitRef*)obj->head.get()) {
        Unit* saved = n->owner;
        if (n->listener)
            n->listener->Notify(8);
        if (n->owner == saved && n->owner) {
            Ref<UnitRef>* pp = (Ref<UnitRef>*)&n->owner->head;
            while (*pp != n)
                pp = &(*pp)->next;
            *pp = n->next;
            n->owner = 0;
            n->next = 0;
        }
    }
}

// FUNCTION: 0x4897b0
void __stdcall NotifyUnitRefs(Unit* obj, int event)
{
    for (UnitRef* n = (UnitRef*)obj->head.get(); n != 0; n = n->next) {
        if (n->listener != 0)
            n->listener->Notify(event);
    }
}

// FUNCTION: 0x4897e0
unsigned char Unit::ChooseWeapon()
{
    unsigned char result;
    if (weapons[0].flags & 2)
        result = 0;
    else if (weapons[1].flags & 2)
        result = 1;
    else
        result = weapons[2].flags & 2;
    return result;
}

#pragma pack(push, 1)
struct UnitDamagePacketOut {
    unsigned char type;
    short victimId;
    short attackerId;
    short amount;
    unsigned char headingByte;
    unsigned char damageKind;
};
#pragma pack(pop)

#include "unit_targets.h"
void __stdcall BroadcastPacket(int who, UnitDamagePacketOut* dmg, int size);
#include "../network/net_game.h"

// FUNCTION: 0x489bb0
void __stdcall DamageUnit(Unit* source, Unit* target, int amount, int type, unsigned short extra)
{
    int dmg;
    if (type != 10) {
        if ((target->activateFlags & 2) && amount < 0x7530)
            amount = (int)(((__int64)((UnitType*)target->def.get())->f1aa * amount) >> 0x10);
        int armour = target->killCount / 5;
        if (armour > 5)
            armour = 5;
        dmg = (25 - armour) * amount * 4 / 100;
    } else {
        dmg = amount;
    }
    UnitDamagePacketOut d;
    d.type = 11;
    d.victimId = !target ? 0 : target->id;
    d.attackerId = !source ? 0 : source->id;
    d.amount = dmg;
    d.headingByte = (unsigned char)(extra >> 8);
    d.damageKind = type;
    ApplyUnitDamage((UnitDamagePacket *)&d);
    if (target->player->active != 0 && target->player->type == 3 && type != 11) {
        if (source)
            BroadcastPacket(source->player->id, &d, 9);
        else
            BroadcastPacket(GetLocalDpid(), &d, 9);
    }
}

#include "../orders/mission_type.h"

#include "../orders/order.h"

#pragma pack(push, 1)
struct UnitDamagePacket {
    unsigned char type;
    unsigned short victimId;
    unsigned short attackerId;
    unsigned short amount;
    unsigned char headingByte;
    unsigned char damageKind;
};
#pragma pack(pop)


#include "../../interfaces/seams.h"
#include "../state/sim_globals.h"
#include "../ingame/game_dialogs.h"
int __cdecl ScaledCos(unsigned short idx, int scale);
int __cdecl ScaledSin(short idx, int scale);

// FUNCTION: 0x489ce0
void __stdcall ApplyUnitDamage(UnitDamagePacket* ev)
{
    Unit* unit = ev->victimId == 0 ? 0 : &g_game->units[ev->victimId];
    Unit* target = ev->attackerId == 0 ? 0 : &g_game->units[ev->attackerId];

    if (unit == 0)
        return;
    if (!(unit->flags & 0x10000000))
        return;
    if (unit->flags & 0x4000)
        return;

    if (ev->damageKind == 10) {
        unit->health = (short)((unsigned int)(unit->health + ev->amount) < ((UnitType*)unit->def.get())->maxhp
                           ? unit->health + ev->amount : ((UnitType*)unit->def.get())->maxhp);
        return;
    }

    g_seams.units->MarkRecentlyDamaged(unit);

    if (ev->damageKind != 11)
        g_seams.ai->ReactToAttack(target, unit, ev->amount);

    unit->lastDamageType = ev->damageKind;

    if (target) {
        unsigned char c = target->playerIndex;
        unit->lastAttackerSlot = c;
        unit->attacker = target;
        if (target->playerIndex == g_game->localPlayer || unit->playerIndex == g_game->localPlayer)
            AddCdActivitySample(1);
    }

    if (ev->damageKind == 2) {
        int ticks = ev->amount;
        if (unit->flags & 0x10000000) {
            if (!(unit->flags & 0x4000)) {
                Player* owner = unit->player;
                if (owner->active && (owner->type == 1 || owner->type == 2)) {
                    if (!(((UnitType*)unit->def.get())->f241 & 0x4000000)) {
                        MissionType kind(s_paralyze_00508d80);
                        Order* e = unit->list;
                        if (e && e->kind == kind) {
                            e->ticks += ticks;
                            return;
                        }
                        g_seams.orders->AppendOrder(unit, new (kSim) Order(kind, 0, 0, ticks, 0, 0));
                        return;
                    }
                }
            }
        }
    } else {
        unit->health -= ev->amount;
        if (unit->health <= 0) {
            if (unit->player->active && (unit->player->type == 1 || unit->player->type == 2)) {
                unit->flags |= 0x4000;
                return;
            }
            unit->health = 0;
        }
        if (ev->damageKind == 1) {
            int a = ScaledCos(ev->headingByte << 8, 400);
            int b = ScaledSin(ev->headingByte << 8, 400);
            g_seams.units->StartScriptWithArgs((CobScript*)unit->script.get(), s_HitByWeapon_00508d74, 0, 0, 2, a, b, 0, 0);
            int pct = unit->health * 100 / ((UnitType*)unit->def.get())->maxhp;
            if (pct < 0)
                pct = 0;
            if (pct > 100)
                pct = 100;
            g_seams.units->StartScriptWithArgs((CobScript*)unit->script.get(), s_TakeDamage_00508d68, 0, 0, 1, pct, 0, 0, 0);
        }
    }
}

// FUNCTION: 0x489fa0
void __stdcall ParalyzeUnit(Unit* unit, int ticks)
{
    if (!(unit->flags & 0x10000000))
        return;
    if (unit->flags & 0x4000)
        return;
    Player* owner = unit->player;
    if (!owner->active)
        return;
    if (owner->type == 1 || owner->type == 2) {
        if (!(((UnitType*)unit->def.get())->f241 & 0x4000000)) {
            MissionType kind("paralyze");
            Order* effect = unit->list;
            if (effect && effect->kind == kind) {
                effect->ticks += ticks;
                return;
            }
            g_seams.orders->AppendOrder(unit, new (kSim) Order(kind, 0, 0, ticks, 0, 0));
        }
    }
}

// FUNCTION: 0x48a060
void __stdcall SetWeaponTargetUnit(char* param_1, char* param_2, int param_3)
{
    char* elem = param_1 + param_3 * 0x1c;
    *(unsigned short*)(elem + 4) = *(unsigned short*)(param_2 + 0xa8);
    *(unsigned short*)(elem + 6) = 0x8000;
    *(unsigned short*)(param_1 + 0xba) &= 0x83ff;
}

// FUNCTION: 0x48a0a0
void __stdcall SetWeaponTargetPos(char* param_1, int* param_2, int param_3)
{
    short* elem = (short*)(param_1 + param_3 * 0x1c + 4);
    elem[0] = (short)(param_2[0] >> 16);
    elem[1] = (short)(param_2[2] >> 16);
    if (elem[1] == (short)0x8000) {
        elem[1] = (short)0x8001;
    }
    *(unsigned short*)(param_1 + 0xba) &= 0x83ff;
}

// FUNCTION: 0x48a0f0
void __stdcall ClearWeaponTarget(Unit* unit, int index)
{
    Point_004898b0* p = (Point_004898b0*)&unit->weapons[index];
    if (p->a != 0 || p->b != (short)0x8000) {
        p->a = 0;
        p->b = (short)0x8000;
        g_seams.units->FindScript((CobScript*)unit->script.get(), "StartBuilding");
        g_seams.units->StartScriptWithArgs((CobScript*)unit->script.get(), "TargetCleared", 0, 0, 1, index, 0, 0, 0);
    }
}

// FUNCTION: 0x48a160
void __stdcall ResetWeaponTarget(Unit* obj, int index)
{
    Point_004898b0* p = (Point_004898b0*)&obj->weapons[index];
    p->a = 0;
    p->b = (short)0x8000;
}

// FUNCTION: 0x48a190
Unit* __stdcall GetWeaponTargetUnit(Unit* obj, int index)
{
    Point_004898b0* p = (Point_004898b0*)&obj->weapons[index];
    if (p->b != (short)0x8000) {
        return 0;
    }
    if (p->a == 0) {
        return 0;
    }
    return &g_game->units[p->a];
}

static Vec3_0048a1e0 offset_0048a1e0(Body_0048a1e0* m, __int64 s)
{
    Vec3_0048a1e0 d;
    d.x = (int)(((__int64)m->offset.x * s) >> 16);
    d.y = (int)(((__int64)m->offset.y * s) >> 16);
    d.z = (int)(((__int64)m->offset.z * s) >> 16);
    return d;
}

// FUNCTION: 0x48a1e0
int __stdcall GetWeaponTargetPos(Unit* unit, Vec3_0048a1e0* pos, int index)
{
    UnitWeaponSlot* e = &unit->weapons[index];
    if (e->aimTargetZOrUnitSentinel != (short)0x8000) {
        pos->x = e->aimTargetXOrUnitId << 16;
        pos->z = e->aimTargetZOrUnitSentinel << 16;
        pos->y = max(g_seams.units->GetGroundHeight((Vec3*)pos), g_game->seaLevel) << 16;
        return 1;
    }
    if (e->aimTargetXOrUnitId == 0) {
        return 0;
    }
    Unit* def = &g_game->units[e->aimTargetXOrUnitId];
    if (def->unitDefIndex == 0) {
        UnitWeaponSlot* f = &unit->weapons[index];
        if (f->aimTargetXOrUnitId != 0 || f->aimTargetZOrUnitSentinel != (short)0x8000) {
            e->aimTargetXOrUnitId = 0;
            e->aimTargetZOrUnitSentinel = (short)0x8000;
            g_seams.units->FindScript((CobScript*)unit->script.get(), "StartBuilding");
            g_seams.units->StartScriptWithArgs((CobScript*)unit->script.get(), "TargetCleared", 0, 0, 1, index, 0, 0, 0);
        }
        return 0;
    }
    g_seams.units->GetSweetSpot(def, (Vec3*)pos);
    if ((e->flags & 2) && !(((Target_0048a1e0*)e->weapon.get())->field_111 & 0x2000000) && def->motion != 0
        && unit->killCount > 5 && ((Target_0048a1e0*)e->weapon.get())->radius != 0) {
        Vec3_0048a1e0 d;
        d.x = unit->pos.x - pos->x;
        d.y = unit->pos.y - pos->y;
        d.z = unit->pos.z - pos->z;
        int dist = (int)sqrt((double)d.x * d.x + (double)d.y * d.y + (double)d.z * d.z);
        int scale = (int)(((__int64)dist << 16) / ((Target_0048a1e0*)e->weapon.get())->radius);
        scale = (int)((scale * (__int64)0xcccc) >> 16);
        __int64 s = scale;
        d = offset_0048a1e0((Body_0048a1e0*)def->motion.get(), s);
        pos->x = pos->x + d.x;
        pos->y = pos->y + d.y;
        pos->z = pos->z + d.z;
    }
    return 1;
}

// FUNCTION: 0x48a440
int __stdcall IsTargetingUnit(Unit* list, Unit* obj)
{
    for (int i = 0; i < 3; i++) {
        if (list->weapons[i].aimTargetZOrUnitSentinel == -0x8000 && list->weapons[i].aimTargetXOrUnitId == obj->id) {
            return 1;
        }
    }
    return 0;
}

#pragma pack(push, 1)
struct MapVertex_0048a490 {
    int x;
    int y;
    int z;
};

struct MapRow_0048a490 {
    char unknown_0[0xc];
    unsigned short* ids;
    char unknown_10[0x20 - 0x10];
};

struct MapInfo_0048a490 {
    char unknown_0[0xc];
    int count;
    char unknown_10[0x24 - 0x10];
    MapVertex_0048a490* verts;
    MapRow_0048a490* rows;
};

struct Pos2_0048a490 {
    int x;
    int z;
};

struct Hs_0048a490 {
    int wx;
    int h;
    int spare;
};
#pragma pack(pop)

unsigned int GetTicks();
int __cdecl AngleOfVector(int x, int y);
void __cdecl RotatePoint2D(unsigned short deg, Pos2_0048a490* p);

// FUNCTION: 0x48a490
void __stdcall AlignUnitToGround(Unit* u)
{
    MapInfo_0048a490* m = ((MapInfo_0048a490 **)g_game->types.get())[u->unitDefIndex];
    MapRow_0048a490* row = m->rows + m->count;
    if (m->count < 0)
        return;
    {
        int gw, c0;
        Pos2_0048a490 t;
        Pos2_0048a490 pts[4];
        Hs_0048a490 hs[4];
        int k;
        for (k = 0; k < 4; k++) {
            MapVertex_0048a490* v = m->verts + row->ids[k];
            int vx = v->x;
            pts[k].x = t.x = vx;
            int vz = v->z;
            t.z = vz;
            pts[k].z = vz;
            RotatePoint2D(u->angles.heading, &t);
            int wx = (short)((t.x + u->pos.x) >> 16);
            int hz = (short)((u->pos.z - t.z) >> 16);
            int fx = wx & 0xf;
            unsigned gz = (unsigned)hz >> 4;
            int fz = hz & 0xf;
            gw = g_game->width;
            unsigned gx = (unsigned)wx >> 4;
            if (gx >= gw - 1)
                return;
            if (gz >= g_game->height - 1)
                return;
            unsigned char* tb = (unsigned char *)g_game->cells.get() + (gz * gw + gx) * 13;
            int b0 = tb[4];
            unsigned char* tb1 = tb + g_game->width * 13;
            int b1 = tb1[4];
            c0 = tb[0x11];
            int c1 = tb1[0x11];
            int H0 = b0 + ((c0 - b0) * fx) / 16;
            int H1 = b1 + ((c1 - b1) * fx) / 16;
            hs[k].wx = wx;
            if ((((UnitType*)u->def.get())->f241 & 0x1000) && (u->flags & 0x10000000)
                && !(u->flags & 0x4000)) {
                hs[k].h = H0 + ((H1 - H0) * fz) / 16;
                int H = hs[k].h;
                int sea = g_game->seaLevel;
                hs[k].h = max(H, sea);
                unsigned int bobClock = g_quirks.hoverBobWallClock ? GetTicks() : g_game->ticks;
                short p = (short)(((bobClock & 0x1f) + k * 8) * 2048 + u->hoverBobPhase);
                int s = ((UnitType*)u->def.get())->sight / 2;
                Body_0048a1e0* o = (Body_0048a1e0*)u->motion.get();
                int q = o->sight;
                if (q >= s)
                    q = s;
                int w = (int)((((__int64)q << 16) / s));
                int mm = 2 - (int)((((__int64)w * 2) >> 16));
                unsigned int n = g_game->ticks - o->age;
                mm -= (unsigned int)(mm * (n < 60 ? n : 60)) / 60;
                hs[k].h = ScaledCos(p, mm) + hs[k].h;
            } else {
                hs[k].h = H0 + ((H1 - H0) * fz) / 16;
            }
            hs[k].spare = hz;
        }
        int h1 = hs[1].h;
        int h0 = hs[0].h;
        int h2 = hs[2].h;
        int h3 = hs[3].h;
        int a = (h0 + h1) / 2;
        int b = (h2 + h3) / 2;
        u->yWhole = (a + b) / 2;
        u->angles.pitch = AngleOfVector(b - a, (short)(abs(pts[0].z - pts[3].z) >> 16));
        u->angles.bank = AngleOfVector(h0 - h1, (short)(abs(pts[0].x - pts[1].x) >> 16));
    }
}
