#include "../game/orders_events.h"
#include "unit_type_set.h"
#include "win_types.h"
#include "unit_def.h"
#include "../../interfaces/ref.h"
#include "../state/block.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

#include "../scripts/cob_script.h"
#include "../util/angles.h"
#include "../util/vec3.h"
#include "../quirks/quirks.h"
#include "../util/tdf.h"
#include "../../interfaces/services.h"
#include "squad.h"
#include "../../interfaces/seams.h"

class UnitRef;


#pragma pack(push, 1)
struct WeaponDefTargetView {
    char unknown_0[0x68];
    int radius;
    char unknown_6c[0x111 - 0x6c];
    unsigned int field_111;
};

struct UnitMotionBodyView {
    char unknown_0[8];
    Vec3 offset;
    char unknown_14[0x20 - 0x14];
    int sight;
    char unknown_24[0x2a - 0x24];
    int age;
};

#include "../network/player.h"

#include "unit.h"

struct ModelGroundView;

#include "../game/game.h"
#include "../state/sim_effects.h"
#pragma pack(pop)

#include "../../link/globals.h"

#include "../debug/debug_lib.h"
#include "../graphics/display.h"

static inline float spd(int v)
{
    return v * 1.52587890625e-05f;
}

// FUNCTION: 0x489280
char* __stdcall MakePropList(UnitDef* obj)
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

    sprintf(p, "%d", obj->buildtime);
    p += strlen(p) + 1;

    sprintf(p, "\n");
    p += strlen(p) + 1;

    if (obj->mobile) {
        sprintf(p, "%.1f %s ", (double)SIM_TICKS_PER_SECOND * spd(obj->maxvelocity) * 0.4,
                Translate("m/s"));
        p += strlen(p) + 1;

        sprintf(p, "%.2f %s", (double)SIM_TICKS_PER_SECOND * spd(obj->acceleration) * 0.4,
                Translate("m/s/s"));
        p += strlen(p) + 1;

        sprintf(p, "%.0f %s",
                (double)SIM_TICKS_PER_SECOND * obj->turnrateUnsigned * 0.0054931640625,
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
    for (int i = 0; i < unit->def->count; i++) {
        if (unit->def->idsSigned[i] == id)
            return 1;
    }
    return 0;
}

#include "unit_ref.h"
#include "../orders/order_base.h"

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

#include "path_order_attach.h"

// FUNCTION: 0x4895c0
PathOrderAttach::PathOrderAttach(Unit* o, OrderBase* v)
    : value(v)
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

Unit* __stdcall UnitRefGet(UnitRef* ref)
{
    return ((PathOrderAttach*)ref)->owner;
}

void __stdcall UnitRefSetValue(UnitRef* ref, OrderBase* value)
{
    ((PathOrderAttach*)ref)->value = value;
}

// FUNCTION: 0x489600 ??_GPathOrderAttach@@UAEPAXI@Z

// FUNCTION: 0x489650
void UnitRef::Unlink()
{
    PathOrderAttach* self = (PathOrderAttach*)this;
    if (self->owner != 0) {
        Ref<PathOrderAttach>* pp = &self->owner->head;
        while (*pp != self)
            pp = &(*pp)->next;
        *pp = self->next;
        self->owner = 0;
        self->next = 0;
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
        OrdersPublishStatusFlags(SCHED_UNITS, saved, listener.get(), 8);
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
            OrdersPublishStatusFlags(SCHED_UNITS, saved, n->listener.get(), 8);
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
        if (n->listener)
            OrdersPublishStatusFlags(SCHED_UNITS, obj, n->listener.get(), event);
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

#include "../network/wire_packets.h"
#include "../../interfaces/seams.h"
#include "../../interfaces/targeting.h"
#include "unit_targets.h"
#include "../network/net_game.h"

#include "../orders/mission_type.h"

#include "../orders/order.h"

#include "../../interfaces/seams.h"
#include "../game/weapons_events.h"
#include "../state/sim_globals.h"
#include "../ingame/game_dialogs.h"
#include "../util/math.h"
#include "../../interfaces/host.h"

static void PublishCleared(Unit* unit, int weapon)
{
    TargetingEventHeader effect;
    effect.kind = TARGETING_EFFECT_CLEARED;
    effect.unit = Ref<Unit>(unit).offset;
    effect.weapon = weapon;
    HarnessTargetingEvent(&effect, sizeof(effect), 1);
    SchedPublish(SCHED_TARGETING, &effect, sizeof(effect));
}

static Unit* UnitOfHandle(unsigned int handle)
{
    Ref<Unit> ref;
    ref.offset = handle;
    return ref.get();
}

// FUNCTION: 0x48a060
static void SetWeaponTargetUnit(Unit* unit, Unit* target, int weapon)
{
    UnitWeaponSlot* elem = &unit->weapons[weapon];
    elem->aimTargetXOrUnitId = target->id;
    elem->aimTargetZOrUnitSentinel = (short)0x8000;
    unit->netDirtyFlags &= 0x83ff;
}

// FUNCTION: 0x48a0a0
static void SetWeaponTargetPos(Unit* unit, const int* pos, int weapon)
{
    UnitWeaponSlot* elem = &unit->weapons[weapon];
    elem->aimTargetXOrUnitId = (short)(pos[0] >> 16);
    elem->aimTargetZOrUnitSentinel = (short)(pos[2] >> 16);
    if (elem->aimTargetZOrUnitSentinel == (short)0x8000) {
        elem->aimTargetZOrUnitSentinel = (short)0x8001;
    }
    unit->netDirtyFlags &= 0x83ff;
}

// FUNCTION: 0x48a0f0
static int ClearWeaponTarget(Unit* unit, int index)
{
    AimTargetPair* p = (AimTargetPair*)&unit->weapons[index];
    if (p->a != 0 || p->b != (short)0x8000) {
        p->a = 0;
        p->b = (short)0x8000;
        PublishCleared(unit, index);
        return 1;
    }
    return 0;
}

// FUNCTION: 0x48a160
static int ResetWeaponTarget(Unit* obj, int index)
{
    AimTargetPair* p = (AimTargetPair*)&obj->weapons[index];
    int had = p->a != 0 || p->b != (short)0x8000;
    p->a = 0;
    p->b = (short)0x8000;
    return had;
}

int __stdcall ApplyTargetingEvent(const TargetingEventHeader* e, unsigned int size)
{
    if (size < sizeof(TargetingEventHeader))
        return 0;
    Unit* unit = UnitOfHandle(e->unit);
    if (!unit || e->weapon < 0 || e->weapon > 2) {
        return 0;
    }
    switch (e->kind) {
    case TARGETING_SET_UNIT: {
        if (size < sizeof(TargetingSetUnit))
            return 0;
        Unit* target = UnitOfHandle(reinterpret_cast<const TargetingSetUnit*>(e)->target);
        if (!target) {
            return 0;
        }
        SetWeaponTargetUnit(unit, target, e->weapon);
        return 1;
    }
    case TARGETING_SET_POS:
        if (size < sizeof(TargetingSetPos))
            return 0;
        SetWeaponTargetPos(unit, reinterpret_cast<const TargetingSetPos*>(e)->pos, e->weapon);
        return 1;
    case TARGETING_CLEAR:
        return ClearWeaponTarget(unit, e->weapon);
    case TARGETING_RESET:
        return ResetWeaponTarget(unit, e->weapon);
    case TARGETING_LOAD: {
        if (size < sizeof(TargetingLoad))
            return 0;
        const TargetingLoad* load = reinterpret_cast<const TargetingLoad*>(e);
        UnitWeaponSlot* elem = &unit->weapons[e->weapon];
        elem->aimTargetXOrUnitId = (short)load->xOrUnit;
        elem->aimTargetZOrUnitSentinel = (short)load->zOrSentinel;
        return 1;
    }
    }
    return 0;
}

// FUNCTION: 0x48a190
Unit* __stdcall GetWeaponTargetUnit(Unit* obj, int index)
{
    AimTargetPair* p = (AimTargetPair*)&obj->weapons[index];
    if (p->b != (short)0x8000) {
        return 0;
    }
    if (p->a == 0) {
        return 0;
    }
    return &g_game->units[p->a];
}

#ifdef BT_REPLAY_INJECT_TARGET_SCALE
#define TARGET_BODY_SCALE BT_REPLAY_INJECT_TARGET_SCALE
#else
#define TARGET_BODY_SCALE 0xcccc
#endif

static Vec3 ScaleBodyOffset(UnitMotionBodyView* m, __int64 s)
{
    Vec3 d;
    d.x = (int)(((__int64)m->offset.x * s) >> 16);
    d.y = (int)(((__int64)m->offset.y * s) >> 16);
    d.z = (int)(((__int64)m->offset.z * s) >> 16);
    return d;
}

// FUNCTION: 0x48a1e0
int __stdcall GetWeaponTargetPos(Unit* unit, Vec3* pos, int index)
{
    UnitWeaponSlot* e = &unit->weapons[index];
    if (e->aimTargetZOrUnitSentinel != (short)0x8000) {
        pos->x = e->aimTargetXOrUnitId << 16;
        pos->z = e->aimTargetZOrUnitSentinel << 16;
        pos->y = 0;
        pos->y = max(g_seams.map->GetGroundHeight((Vec3*)pos), g_game->seaLevel) << 16;
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
            PublishCleared(unit, index);
        }
        return 0;
    }
    g_seams.scripts->GetSweetSpot(def, (Vec3*)pos);
    if ((e->flags & 2) && !(((WeaponDefTargetView*)e->weapon.get())->field_111 & 0x2000000) && def->motion != 0
        && unit->killCount > 5 && ((WeaponDefTargetView*)e->weapon.get())->radius != 0) {
        Vec3 d;
        d.x = unit->pos.x - pos->x;
        d.y = unit->pos.y - pos->y;
        d.z = unit->pos.z - pos->z;
        int dist = (int)sqrt((double)d.x * d.x + (double)d.y * d.y + (double)d.z * d.z);
        int scale = (int)(((__int64)dist << 16) / ((WeaponDefTargetView*)e->weapon.get())->radius);
        scale = (int)((scale * (__int64)TARGET_BODY_SCALE) >> 16);
        __int64 s = scale;
        d = ScaleBodyOffset((UnitMotionBodyView*)def->motion.get(), s);
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
struct ModelGroundPrimitive {
    char unknown_0[0xc];
    unsigned short* ids;
    char unknown_10[0x20 - 0x10];
};

struct ModelGroundView {
    char unknown_0[0xc];
    int count;
    char unknown_10[0x24 - 0x10];
    Vec3* verts;
    ModelGroundPrimitive* rows;
};

struct XzPoint {
    int x;
    int z;
};

struct CornerHeightSample {
    int wx;
    int h;
    int spare;
};
#pragma pack(pop)

int __cdecl AngleOfVector(int x, int y);
void __cdecl RotatePoint2D(unsigned short deg, XzPoint* p);

// FUNCTION: 0x48a490
void __stdcall AlignUnitToGround(Unit* u)
{
    ModelGroundView* m = ((ModelGroundView **)g_game->types.get())[u->unitDefIndex];
    ModelGroundPrimitive* row = m->rows + m->count;
    if (m->count < 0)
        return;
    {
        int gw, c0;
        XzPoint t;
        XzPoint pts[4];
        CornerHeightSample hs[4];
        int k;
        for (k = 0; k < 4; k++) {
            Vec3* v = m->verts + row->ids[k];
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
            if ((u->def->flags1 & 0x1000) && (u->flags & 0x10000000)
                && !(u->flags & 0x4000)) {
                hs[k].h = H0 + ((H1 - H0) * fz) / 16;
                int H = hs[k].h;
                int sea = g_game->seaLevel;
                hs[k].h = max(H, sea);
                unsigned int bobClock = g_quirks.hoverBobWallClock ? host_time_ms() * SIM_TICKS_PER_SECOND / 1000 : g_game->ticks;
                short p = (short)(((bobClock & 0x1f) + k * 8) * 2048 + u->hoverBobPhase);
                int s = u->def->maxvelocity / 2;
                UnitMotionBodyView* o = (UnitMotionBodyView*)u->motion.get();
                int q = o->sight;
                if (q >= s)
                    q = s;
                int w = (int)((((__int64)q << 16) / s));
                int mm = 2 - (int)((((__int64)w * 2) >> 16));
                unsigned int n = g_game->ticks - o->age;
                mm -= (unsigned int)(mm * (n < 60 ? n : 60)) / 60;
                hs[k].h = ScaledCos((unsigned short)p, mm) + hs[k].h;
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

// FUNCTION: 0x40b7b0
Unit* __stdcall FindWeaponTarget(Unit* unit,unsigned char weapon,int useRange)
{
    int ai=unit->player->active && unit->player->type==2;
    Unit* fallback=0;
    int fallbackDistance=0x7fffffff;
    int bestDistance=0x7fffffff;
    Unit* best=0;
    std::vector<Unit*> candidates;
    if(useRange) g_seams.los_radar->GetVisibleEnemiesInRadius(unit->playerIndex,(const Vec3*)&unit->pos,g_seams.weapons->GetWeaponRange(unit,weapon),0,&candidates);
    else g_seams.los_radar->GetVisibleEnemiesInRadius(unit->playerIndex,(const Vec3*)&unit->pos,unit->def->range,0,&candidates);
    for(int count=0;count<50;++count) {
        if(candidates.empty()) break;
        std::vector<Unit*>::iterator it=candidates.begin()+RandomInt(candidates.size());
        Unit* target=*it;
        ((UnitSwapEraseVector*)&candidates)->EraseSwapBack(it);
        if((target->flags&0x10000000) && !(target->flags&0x4000) &&
           ((target->def->flags1&0x8000) || ai || (g_game->matchFlags&4)) &&
           ((unit->def->flags1&0x10000000) || g_seams.weapons->WeaponCanReachUnit(unit,target,weapon)) &&
           (useRange || !unit->def->categories->Contains(target->unitDefIndex)) &&
           (!((*(unsigned char*)&unit->weapons[weapon].weapon->flags)&0x80) || !(target->activateFlags&0x10))) {
            int dz=unit->pos.z-target->pos.z;
            int dx=unit->pos.x-target->pos.x;
            int d=RandomInt((int)(((__int64)dx*dx)>>32)+(int)(((__int64)dz*dz)>>32));
            if(unit->def->weaponCategories[weapon]->Contains(target->unitDefIndex)) {
                if(d<fallbackDistance) { fallbackDistance=d; fallback=target; }
            } else if(d<bestDistance) { bestDistance=d; best=target; }
        }
    }
    if(best) return best;
    return fallback;
}

// FUNCTION: 0x43b700
Unit* __stdcall FindBestTargetIfFireAtWill(Unit* unit)
{
    if (unit->fireOrder == 2)
        return FindWeaponTarget(unit, 0, 0);
    return 0;
}
