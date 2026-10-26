#include <stdio.h>
#include "../state/block.h"
#include "../../interfaces/ref.h"
#include <string.h>
#include <stdlib.h>
#include <minmax.h>
#include <math.h>
#include <vector>
#include <new>
#include "../state/sim_allocator.h"
#include "win_types.h"


#pragma pack(push, 1)

class CommandArgs {
public:
    char unknown_0[0xd0];
    union {
        int count;
    };
    char* GetArg(int index, char* fallback);
    float GetFloatArg(int index, float default_val);
    int GetIntArg(int index, int fallback);
    CommandArgs* InitArgs();
};

#include "../units/unit_type_set.h"

#include "../weapons/weapon_def.h"

#include "../weapons/unit_weapon_slot.h"

#include "../orders/order.h"

#include "../util/vec3.h"

static inline int Vec3Length(const Vec3* v)
{
    double fx = v->x;
    double fy = v->y;
    double fz = v->z;
    return (int)sqrt(fx * fx + fy * fy + fz * fz);
}

static inline void Vec3Scale(Vec3* v, int s)
{
    v->x = (int)(((__int64)v->x * s) >> 16);
    v->y = (int)(((__int64)v->y * s) >> 16);
    v->z = (int)(((__int64)v->z * s) >> 16);
}

static inline Vec3 PosPlus(const Vec3& a, const Vec3& b)
{
    Vec3 r;
    r.x = a.x + b.x;
    r.y = a.y + b.y;
    r.z = a.z + b.z;
    return r;
}
static inline Vec3 PosMinus(const Vec3& a, const Vec3& b)
{
    Vec3 r;
    r.x = a.x - b.x;
    r.y = a.y - b.y;
    r.z = a.z - b.z;
    return r;
}

static inline Vec3 MakeVec3(int x, int y, int z)
{
    Vec3 v;
    v.x = x;
    v.y = y;
    v.z = z;
    return v;
}

struct Sub_00409520 {
    char unknown_0[0xd4];
    unsigned short damage;
    char unknown_d6[6];
    int range;
    char unknown_e0[0x2a];
    char index;
};

struct Bits {
    unsigned* words;
    int Test(unsigned short index) const { return (words[index>>5]&(1u<<(index&31)))!=0; }
};

#include "../units/unit_def.h"

struct Flags241 {
    unsigned int unknown_241 : 5;
    unsigned int downloadable : 1;
    unsigned int builder : 1;
    unsigned int unused7 : 4;
    unsigned int flying : 1;
    unsigned int unused12 : 20;
};
struct Flags245 {
    unsigned int unknown_bits : 4;
    unsigned int flag4 : 1;
    unsigned int unused5 : 7;
    unsigned int special : 1;
    unsigned int unused13 : 19;
};

struct Player;
struct _iobuf;
struct Group;
struct Economy;
class SquadTimer;

#include "../units/unit.h"

struct Sound;
struct HapiBank;
struct TdfFile;
struct TdfRecord;
struct Gadget;
struct Layer;
struct UnitSync;
struct Packet;
struct Pathfinder;
#include "../network/player.h"
inline int Unit::Ready() { return (flags & 0x10000000) && !(flags & 0x4000); }

struct Economy {
    char unknown_0[0x8c];
    float energy;
    float energyIncome;
    float energyUsage;
    float cost;
};

#include "group.h"

void __stdcall OrderSquad(Player*, int, unsigned char, int, Unit*, Vec3*, int, int);

#include "squad_manager.h"

struct Target_00406f50 {
    char unknown_0[0xbb];
    unsigned short bit0_4 : 5;
    unsigned short flag5 : 1;
    unsigned short flag6 : 1;
    unsigned short bit7_15 : 9;
};

#include "../weapons/projectile.h"

#include "squad_timer.h"

#include "../graphics/movies.h"
int GetCobChecksum(int);

void RegisterUnitOrders(void);
#include "../ingame/control_panel.h"


#include "../orders/mission_type.h"

struct Feature {
    char unknown_0[0xf0];
    float value;
    char unknown_f4[0xfe - 0xf4];
    unsigned short flags;
};

#include "../map/cell.h"

#include "../map/mission.h"

#include "../game/game.h"

#include "player_ai.h"

#include "vec.h"

struct Vec3_40b0d0 {
    int x;
    int y;
    int z;
};

struct Struct_0040ba80 {
    int a, b, c;
};

struct Struct_0040bab0 {
    int a, b, c;
};

struct Vec3_0040beb0 {
    int x;
    int y;
    int z;

    int Length() const
    {
        double fx = x;
        double fy = y;
        double fz = z;
        return (int)sqrt(fx * fx + fy * fy + fz * fz);
    }
    void Scale(int s)
    {
        x = (int)(((__int64)x * s) >> 16);
        y = (int)(((__int64)y * s) >> 16);
        z = (int)(((__int64)z * s) >> 16);
    }
};

static inline Vec3_0040beb0 operator-(const Vec3_0040beb0& p, const Vec3_0040beb0& q)
{
    Vec3_0040beb0 r;
    r.x = p.x - q.x;
    r.y = p.y - q.y;
    r.z = p.z - q.z;
    return r;
}

struct Class_004800c0 {
public:
    void EraseSwapBack(Unit**);
};

#pragma pack(pop)

#include "../../link/globals.h"

void __stdcall ScaleUnitWeights(int player, UnitTypeSet* set, float value, int count);
void __stdcall SetUnitLimits(int player, UnitTypeSet* set, int value, int param_4);
#include "ai_player.h"
typedef void (__stdcall* Command_00406f00)(CommandArgs* args);
void __stdcall RegisterCommand(const char* name, Command_00406f00 fn, int flags);
#include "../../interfaces/seams.h"
#include "../quirks/quirks.h"
#include "../state/sim_globals.h"
#include "../util/file_printf.h"
#include "../orders/unit_orders.h"
#include "../util/math.h"
#include "../sound/sound.h"
#include "../units/squads.h"
void __stdcall GetBasePosition(int index, Vec3* out);
int __cdecl ScaledSin(short angle, int scale);
int __cdecl ScaledCos(short angle, int scale);
int __cdecl ScaledSin(int angle, int distance);
int __cdecl ScaledCos(int angle, int distance);
int __stdcall IsPointVisible(Player* player, Vec3Words* pos);
int __stdcall SumUnitRatingsInRange(int index, Vec3* pos, int range);
#include "pathfind.h"
void __stdcall PopHeapFirst(Elem_0040cc40* first, Elem_0040cc40* last, Elem_0040cc40* dest,
                            Elem_0040cc40 val, int*);

#include "../util/commands.h"
int __stdcall GetWeaponRange(Unit*, unsigned char);
void __stdcall GetVisibleEnemiesInRadius(int player, const Vec* pos, int radius, int flags, std::vector<Unit*>* out);
int __stdcall IsCountBelowAiLimit(int, unsigned short, int);
void __stdcall GetBasePosition(int index, Struct_0040ba80* out);

static const float Zero_004fc968 = 0.0f;

static inline int Contains(unsigned int* bits, unsigned short index)
{
    return bits[index >> 5] & (1 << (index & 31));
}

static inline Vec3 Direction(short angle, int scale)
{
    Vec3 v;
    v.x = -ScaledSin(angle, scale);
    v.y = 0;
    v.z = -ScaledCos(angle, scale);
    return v;
}

static inline Point16 WorldToCell(Vec3 v, Point16 origin)
{
    Point16 c;
    c.x = (v.x - (origin.x << 19) + 0x80000) >> 20;
    c.y = (v.z - (origin.y << 19) + 0x80000) >> 20;
    return c;
}

static inline int DistSq(const Point16& a, const Point16& b)
{
    int dy = a.y - b.y;
    int dx = a.x - b.x;
    return dx * dx + dy * dy;
}

static inline void PopHeap(Elem_0040cc40* f, Elem_0040cc40* l)
{
    PopHeapFirst(f, l - 1, l - 1, Elem_0040cc40(*(l - 1)), (int*)0);
}

static inline float Max(float a, float b) { return a > b ? a : b; }

static inline Vec3 MoveTowards(const Vec3* from, const Vec3* to, int maxLen)
{
    Vec3 d = PosMinus(*to, *from);
    int len = Vec3Length(&d);
    if (maxLen >= len)
        return *to;
    Vec3Scale(&d, (int)(((__int64)maxLen << 16) / len));
    Vec3 r;
    r.x = from->x + d.x;
    r.y = from->y + d.y;
    r.z = from->z + d.z;
    return r;
}

static inline void CellToWorld(Vec3* out, Point16 p, Point16 origin) {
    out->x=(origin.x+p.x*2)<<19;
    out->z=(origin.y+p.y*2)<<19;
}

void Group::Send(unsigned char mode, int remove, Unit* target, Vec3* pos, int flags, int extra)
{
    OrderSquad(player, id, mode, remove, target, pos, flags, extra);
}

// FUNCTION: 0x406c90
void __stdcall CmdPlan(CommandArgs* args)
{
    g_aiCommandsEnabled = 0;
    for (int i = 1; i < args->count; ++i) {
        if (!_strcmpi(args->GetArg(1, DAT_005119b8), "any")) g_aiCommandsEnabled = 1;
        if (g_game->difficulty == 0 && !_strcmpi(args->GetArg(i, DAT_005119b8), "easy")) g_aiCommandsEnabled = 1;
        if (g_game->difficulty == 1 && !_strcmpi(args->GetArg(i, DAT_005119b8), "medium")) g_aiCommandsEnabled = 1;
        if (g_game->difficulty == 2 && !_strcmpi(args->GetArg(i, DAT_005119b8), "hard")) g_aiCommandsEnabled = 1;
    }
}

#pragma auto_inline(off)
// FUNCTION: 0x406da0
void EnableAICommands()
{
    g_aiCommandsEnabled = 1;
}
#pragma auto_inline(on)

// FUNCTION: 0x406db0
void __stdcall CmdWeight(CommandArgs* args)
{
    if (g_aiCommandsEnabled != 0) {
        int count;
        UnitTypeSet set;
        memset(&set, 0, sizeof(set));
        g_seams.units->UnitTypeSetAddTypeOrCategory(&set, args->GetArg(1, DAT_005119b8), &count);
        float value = args->GetFloatArg(2, 0);
        for (char i = 0; i < 10; i++) {
            if (g_game->players[i].ai != 0) {
                ScaleUnitWeights(i, &set, value, count);
            }
        }
    }
}

// FUNCTION: 0x406e40
void __stdcall CmdLimit(CommandArgs* args)
{
    if (g_aiCommandsEnabled != 0) {
        int count;
        UnitTypeSet set;
        memset(&set, 0, sizeof(set));
        g_seams.units->UnitTypeSetAddTypeOrCategory(&set, args->GetArg(1, DAT_005119b8), &count);
        int value = args->GetIntArg(2, 0);
        for (char i = 0; i < 10; i++) {
            if (g_game->players[i].active != 0 && g_game->players[i].type == 2) {
                SetUnitLimits(i, &set, value, count);
            }
        }
    }
}

// FUNCTION: 0x406ee0
int __stdcall IsCountBelowAiLimit(unsigned char player, unsigned short id, int value)
{
    return IsUnderLimit(player, id, value);
}

// FUNCTION: 0x406f00
void RegisterAICommands()
{
    RegisterCommand("plan", CmdPlan, 8);
    RegisterCommand("weight", CmdWeight, 8);
    RegisterCommand("limit", CmdLimit, 8);
}

// FUNCTION: 0x406f40
void FUN_00406f40(void)
{
}

// FUNCTION: 0x406f50
void SquadManager::MarkOwnerNetDirtyFromDamageSplit(Projectile* projectile, int a, int b)
{
    Target_00406f50* t = (Target_00406f50*)projectile->attacker.get();
    if (t) {
        if (a > b * 2) {
            t->flag6 = 1;
            return;
        }
        t->flag5 = 1;
    }
}

// FUNCTION: 0x406f80
void __stdcall ReactToAttack(Unit* attacker, Unit* unit, int unused)
{
    g_seams.units->NotifyUnitRefs(unit,16);
    if (attacker && !attacker->unitDefIndex) attacker=0;
    if ((unit->def->flags2&0x1000) && unit->player->active && unit->player->type==2) {
        unit->player->ai->nextAction=RandomInt(300)+(unsigned int)g_game->ticks+30;
        g_seams.orders->DeleteOrders(unit,0);
    }
    if (attacker && unit->player->active && (unit->player->type==1 || unit->player->type==2) &&
        (unit->def->flags1&0x10010000) && unit->buildLeft==Zero_004fc968 && !unit->player->allied[attacker->player->index]) {
        int ordered=0;
        if ((!unit->list || (unit->list->flags_42&0x20000)) &&
            !Contains(unit->def->categories,attacker->unitDefIndex) &&
            !Contains(unit->def->weaponCategories[0],attacker->unitDefIndex) && g_seams.weapons->WeaponCanReachUnit(unit,attacker,0))
            ordered=g_seams.orders->IssueAttackOrder(unit,attacker,0);
        if (!ordered && (unit->flags&0x300000)) {
            for (unsigned char i=0;i<3;++i) {
                UnitWeaponSlot* weapon=&unit->weapons[i];
                if ((weapon->flags&2) && (weapon->flags&0x10) && g_seams.weapons->WeaponCanReachUnit(unit,attacker,i) &&
                    !((unsigned char)(weapon->weapon->flags.value>>26)&1)) {
                    Unit* target=g_seams.targeting->GetWeaponTargetUnit(unit,i);
                    if (!target || !g_seams.weapons->WeaponCanReachUnit(unit,target,i) || Contains(unit->def->weaponCategories[i],target->unitDefIndex))
                        g_seams.targeting->SetWeaponTargetUnit(unit,attacker,i);
                }
            }
        }
    }
    if (!(g_seams.orders->GetOrderFlags(unit)&0x80) && (unit->lastAttackerSlot!=unit->playerIndex || unit->lastDamageType==1))
        QueueUnitSpeechIfNotVisible(unit,2,0);
}

// FUNCTION: 0x4071f0
Unit* SquadManager::FindNearestEnemyUnit(int x,int y,int z)
{
    int best=0x7fffffff;
    Unit* result=0;
    for(unsigned char i=0;i<10;++i) {
        Player* p=&g_game->players[i];
        if(i>=10) continue;
        if(p->active && (p->type==1 || p->type==2 || p->type==3) && p->index!=10 && !player->allied[p->index]) {
            Unit* u=p->unitsBegin;
            Unit* last=p->unitsEnd;
            for(;u<=last;++u) {
                if((u->flags&0x10000000) && u->mode!=2 && !(u->flags&0x8000) && !(*(unsigned short*)&u->activateFlags&4)) {
                    int dz=z-u->pos.z;
                    int dx=x-u->pos.x;
                    int distance=(int)(((__int64)dx*dx)>>32)+(int)(((__int64)dz*dz)>>32);
                    if(distance<best) { result=u; best=distance; }
                }
            }
        }
    }
    return result;
}

// FUNCTION: 0x407350
SquadTimer::SquadTimer(SquadManager* p, Group* q)
    : owner(p), group(q), next(0), player(p->index)
{
}

// FUNCTION: 0x407380
void SquadTimer::OnTimer()
{
}

// FUNCTION: 0x407390 ??_GSquadTimer@@UAEPAXI@Z

// FUNCTION: 0x4073b0
int SquadTimer::TryGetBaseBuildOrScoutCentroid(Vec3* out)
{
    if (owner->timers[5]->GetAveragePosition(out))
        return 1;
    if (owner->timers[1]->GetAveragePosition(out))
        return 1;
    return owner->timers[4]->GetAveragePosition(out) != 0;
}

// FUNCTION: 0x407410
int SquadTimer::GetAveragePosition(Vec3* out)
{
    Group* g = group;
    int n = g->units.size();
    if (n == 0)
        return 0;
    int x = 0, y = 0, z = 0;
    for (SimVector<Unit*>::iterator it = g->units.begin(); it != g->units.end(); ++it) {
        x += (*it)->xWhole;
        y += (*it)->yWhole;
        z += (*it)->zWhole;
    }
    *out = MakeVec3(x / n << 16, y / n << 16, z / n << 16);
    return 1;
}

// FUNCTION: 0x4074a0
int SquadTimer::CountGroupUnitsInRadius(Vec3* pos, int radius)
{
    int count = 0;
    int squared = radius * radius;
    Unit* u = group->player->unitsBegin;
    Unit* last = group->player->unitsEnd;
    for (; u <= last; ++u) {
        if (u->group == group->id) {
            int dz = pos->z - u->pos.z;
            int dx = pos->x - u->pos.x;
            int distance = (int)(((__int64)dx * dx) >> 32) + (int)(((__int64)dz * dz) >> 32);
            if (distance <= squared)
                ++count;
        }
    }
    return count;
}

// FUNCTION: 0x407560
void AssaultTimer::RebalanceAssaultGroupByCentroid(int kind, int limit)
{
    SquadTimer* other = owner->timers[kind];
    if (other == this)
        return;
    if (group->units.empty()) {
        if (other->group->units.empty())
            return;
        SetUnitSquad(other->group->units[0], group->id);
    }
    int sx = 0, sz = 0;
    SimVector<Unit*>::iterator it;
    for (it = group->units.begin(); it != group->units.end(); ++it) {
        sx += (*it)->xWhole;
        sz += (*it)->zWhole;
    }
    int cx, cz;
    while (1) {
        Group* g = group;
        cx = sx / (int)g->units.size();
        cz = sz / (int)g->units.size();
        if (g->units.size() == 1)
            break;
        int maxd = 0;
        SimVector<Unit*>::iterator best = g->units.end();
        for (it = g->units.begin(); it != g->units.end(); ++it) {
            int dx = (*it)->xWhole - cx;
            int dz = (*it)->zWhole - cz;
            int d = dx * dx + dz * dz;
            if (d > maxd) {
                maxd = d;
                best = it;
            }
        }
        if (maxd < limit * (int)g->units.size())
            break;
        SetUnitSquad(*best, kind);
        sx -= (*best)->xWhole;
        sz -= (*best)->zWhole;
    }
    std::vector<Unit*> list;
    for (it = other->group->units.begin(); it != other->group->units.end(); ++it) {
        int dx = (*it)->xWhole - cx;
        int dz = (*it)->zWhole - cz;
        int d = dx * dx + dz * dz;
        if (d < limit * (int)group->units.size())
            list.push_back(*it);
    }
    for (it = list.begin(); it != list.end(); ++it)
        SetUnitSquad(*it, group->id);
}

static inline int Rally(Ref<SquadManager>& owner, Vec3* pos)
{
    if (owner->timers[5]->GetAveragePosition(pos))
        return 1;
    if (owner->timers[1]->GetAveragePosition(pos))
        return 1;
    if (owner->timers[4]->GetAveragePosition(pos))
        return 1;
    return 0;
}

// FUNCTION: 0x4077e0
void AssaultTimer::OnTimer()
{
    Vec3 pos;
    Vec3 retreat;
    next = (unsigned int)g_game->ticks + 300;
    RebalanceAssaultGroupByCentroid(kind, limit);
    if (!group->units.empty()) {
        if ((int)group->units.size() <= minimum || (!attacking && (int)group->units.size() < maximum)) {
            if (Rally(owner, &retreat)) {
                attacking = 0;
                group->Send(2, 0, 0, &retreat, 160, 0);
                return;
            }
        }
        attacking = 1;
        GetAveragePosition(&pos);
        Unit* target = owner->FindNearestEnemyUnit(pos);
        if (target)
            group->Send(3, 0, target, 0, 0, 0);
    }
}

// FUNCTION: 0x407930
// FUNCTION: 0x407980 ??_GAssaultTimer@@UAEPAXI@Z
AssaultTimer::AssaultTimer(SquadManager* p, Group* q, int a, int b)
    : SquadTimer(p, q), limit(b), kind(a)
{
    attacking = 0;
    maximum = 6;
    minimum = 3;
}

// FUNCTION: 0x4079a0
// FUNCTION: 0x4079d0 ??_GEscortTimer@@UAEPAXI@Z
EscortTimer::EscortTimer(SquadManager* p, Group* q, int a)
    : SquadTimer(p, q), other(a)
{
}

// FUNCTION: 0x4079f0
void EscortTimer::OnTimer()
{
    next = (unsigned int)g_game->ticks + 150;
    SquadTimer* o = owner->timers[other];
    if (!group->units.empty()
        && !o->group->units.empty()) {
        Vec3 pos;
        if (o->GetAveragePosition(&pos)) {
            Group* g = group;
            OrderSquad(g->player, g->id, 2, 0, 0, &pos, 0, 0);
        }
    }
}

// FUNCTION: 0x407a90
// FUNCTION: 0x407ac0 ??_GSquadScoutTimer@@UAEPAXI@Z
SquadScoutTimer::SquadScoutTimer(SquadManager* p, Group* q)
    : SquadTimer(p, q)
{
}

struct FixedParts_00407ae0 {
    unsigned int frac : 16;
    int whole : 16;
};

union Fixed_00407ae0 {
    int value;
    FixedParts_00407ae0 parts;
};

static inline Fixed_00407ae0 MakeFixed(int i)
{
    Fixed_00407ae0 f;
    f.parts.frac = 0;
    f.parts.whole = i;
    return f;
}

void __stdcall GetBasePosition(int index, Vec3* out);

static inline Vec3 GetRallyPoint(int index)
{
    Vec3 p;
    GetBasePosition(index, &p);
    return p;
}

// FUNCTION: 0x407ae0
void SquadScoutTimer::OnTimer()
{
    Vec3 dest;
    int delay = RandomInt(900) + 30;
    next = (unsigned int)g_game->ticks + delay;
    if ((int)group->units.size() < 5) {
        Vec3 pos = GetRallyPoint(player);
        if ((((Vec3Words*)&pos)->xWhole | ((Vec3Words*)&pos)->zWhole) == 0) {
            GetAveragePosition(&dest);
            Unit* target = owner->FindNearestEnemyUnit(dest);
            if (g_quirks.patrolNullEnemySkip && target == 0)
                return;
            group->Send(9, 1, 0, &target->pos, 0, 0);
        } else {
            int n = RandomInt(2) + 2;
            int w = g_game->baseX / 8, h = g_game->baseY / 8;
            for (int i = 0; i < n; i++) {
                int dx = (RandomInt(w) - w / 2) << 16;
                dest.x = pos.x + dx;
                dest.y = pos.y;
                int dz = (RandomInt(h) - h / 2) << 16;
                dest.z = pos.z + dz;
                if (i == 0)
                    group->Send(2, 0, 0, &dest, 0, 0);
                else
                    group->Send(9, 1, 0, &dest, 0, 0);
            }
        }
    } else {
        dest.y = 0;
        if (RandomInt(2)) {
            dest.x = RandomInt(g_game->baseX) << 16;
            dest.z = (RandomInt(2) ? MakeFixed(0) : MakeFixed(g_game->baseY - 1)).value;
        } else {
            dest.x = (RandomInt(2) ? MakeFixed(0) : MakeFixed(g_game->baseX - 1)).value;
            dest.z = RandomInt(g_game->baseY) << 16;
        }
        group->Send(9, 0, 0, &dest, 0, 0);
    }
}

// FUNCTION: 0x407d40
// FUNCTION: 0x407e70 ??_GSpatialTimer@@UAEPAXI@Z
SpatialTimer::SpatialTimer(SquadManager* p, void* q)
    : SquadTimer(p, (Group*)q)
{
    int x = (int)(g_game->baseX / 2 * 65536.0);
    best = MakeVec3(x, 0, (int)(g_game->baseY / 2 * 65536.0));
    x = (int)(g_game->baseX / 2 * 65536.0);
    probe = MakeVec3(x, 0, (int)(g_game->baseY / 2 * 65536.0));
    x = (int)(g_game->baseX / 2 * 65536.0);
    step = MakeVec3(x, 0, (int)(g_game->baseY / 2 * 65536.0));
    bestRating = 0;
}

static inline int IsExplored(Player* player, Vec3Words* pos)
{
    int tx = pos->xWhole >> 5;
    int ty = (pos->zWhole - (pos->yWhole >> 1)) >> 5;
    if ((unsigned int)tx < (unsigned int)player->exploredWidth && (unsigned int)ty < (unsigned int)player->exploredHeight &&
        player->explored[(unsigned int)player->exploredWidth * ty + tx])
        return 1;
    return 0;
}

static inline int IsVisible(Player* player, Vec3Words* pos)
{
    if ((g_game->mapFlags & 2) == 2)
        return IsExplored(player, pos);
    return IsPointVisible(player, pos);
}

static inline Vec3 Offset(int angle, int distance)
{
    Vec3 v;
    v.x = -ScaledSin(angle, distance);
    v.y = 0;
    v.z = -ScaledCos(angle, distance);
    return v;
}

// FUNCTION: 0x407e90
void SpatialTimer::OnTimer()
{
    int delay = RandomInt(150) + 30;
    next = g_game->ticks + delay;
    if (group->units.empty())
        return;
    std::vector<Unit*> unused;
    if (RandomInt(10) == 0) {
        probe = best;
        int angle = RandomInt(0x10000);
        step = Offset(angle, 0x1400000);
    }
    probe += step;
    if (IsVisible((Player*)group->player, (Vec3Words*)&probe)) {
        int r = SumUnitRatingsInRange(player, &probe, 0xa0);
        if (RandomInt(r) > RandomInt(bestRating)) {
            bestRating = r;
            best = probe;
        }
    }
    for (SimVector<Unit*>::iterator it = group->units.begin();
         it != group->units.end(); ++it) {
        Unit* u = *it;
        if (u->def->c04) {
            if (u->motion || g_seams.weapons->WeaponCanReachPos(u, (Vec3*)&u->pos, (Vec3*)&best, 0)) {
                MissionType kind = g_seams.orders->GetOrderType(3, u, nullptr, (Vec3*)&best);
                if (kind.index)
                    g_seams.orders->AddOrder(kind, 0, u, nullptr, (Vec3*)&best, 0, 0);
            }
        }
    }
}

// FUNCTION: 0x408090
int __stdcall IsPointVisible(Player* player, Vec3Words* pos)
{
    int tx = pos->xWhole >> 5;
    int ty = (pos->zWhole - (pos->yWhole >> 1)) >> 5;
    if (!((unsigned int)tx < (unsigned int)player->exploredWidth && (unsigned int)ty < (unsigned int)player->exploredHeight)) {
        return 0;
    }
    return (g_game->visibilityMask[(unsigned int)player->exploredWidth * ty + tx] & (1 << g_game->playerIndex)) != 0;
}

static inline int FixMul(int a, int b)
{
    return (int)(((__int64)a * b) >> 16);
}

static inline int FixDiv(int a, int b)
{
    return (int)(((__int64)a << 16) / b);
}

static inline int MapRange()
{
    return (g_game->mapWidth + g_game->mapHeight) / 3 << 16;
}

static inline int IsFlag12(const UnitDef* def)
{
    return def->flags2 & 0x1000;
}

// FUNCTION: 0x408100
void ScoutTimer::OnTimer()
{
    next = (unsigned int)g_game->ticks + 90;
    Vec3 origin;
    GetBasePosition(player, &origin);
    SimVector<Unit*>::iterator it;
    for (it = group->units.begin(); it != group->units.end(); ++it) {
        Unit* u = *it;
        if (u->def->count
            && (!IsFlag12(u->def.get())
                || (GetBuilderCount(player) < 5 && (unsigned int)g_game->ticks >= (unsigned int)owner->nextAction))
            && (!u->list || !(u->list->flags_42 & 8))) {
            unsigned short idx = ChooseBuildOption(player, u);
            if (idx) {
                Vec3 pos;
                pos.x = 0;
                pos.y = 0;
                pos.z = 0;
                int ok = FindBuildPosition(player, &u->pos, &g_game->defs[idx], &pos);
                if (IsFlag12(u->def.get())) {
                    int range = MapRange();
                    origin.y = pos.y;
                    Vec3 offset = pos - origin;
                    int len = Vec3Length(&offset);
                    if (len > range || len > range)
                        ok = 0;
                }
                if (ok) {
                    MissionType kind = g_seams.orders->GetOrderType(0xe, u, nullptr, &pos);
                    g_seams.orders->AddOrder(kind, 0, u, nullptr, &pos, idx, 1);
                }
            }
        }
    }
    for (it = group->units.begin(); it != group->units.end(); ++it) {
        Unit* u = *it;
        Vec3 target;
        if ((!u->list || (u->list->flags_42 & 0x4000))
            && (!IsFlag12(u->def.get()) || GetBuilderCount(player) >= 5)) {
            target = origin;
            if (IsFlag12(u->def.get())) {
                origin.y = u->pos.y;
                Vec3 d = origin - u->pos;
                if (Vec3Length(&d) > 0x2800000)
                    d = Direction(RandomInt(0x10000), 0x2800000);
                target = origin + d;
                MissionType kind;
                kind = g_seams.orders->GetOrderType(2, u, nullptr, &target);
                g_seams.orders->AddOrder(kind, 0, u, nullptr, &target, 0, 0);
                kind = g_seams.orders->GetOrderType(9, u, nullptr, &origin);
                g_seams.orders->AddOrder(kind, 1, u, nullptr, &origin, 0, 0);
            } else {
                Vec3 d = origin - u->pos;
                int len = Vec3Length(&d);
                if (len < 0x1400000) {
                    if (len < 0x100000) {
                        d = Direction(RandomInt(0x10000), 0x1400000);
                    } else {
                        int s = FixDiv(0x1400000, len);
                        d.x = FixMul(s, d.x);
                        d.y = FixMul(d.y, s);
                        d.z = FixMul(d.z, s);
                    }
                    target = u->pos + d;
                }
                MissionType kind = g_seams.orders->GetOrderType(9, u, nullptr, &target);
                g_seams.orders->AddOrder(kind, 0, u, nullptr, &target, 0, 0);
            }
        }
    }
}

// FUNCTION: 0x4085d0
// FUNCTION: 0x408600 ??_GScoutTimer@@UAEPAXI@Z
ScoutTimer::ScoutTimer(SquadManager* p, Group* q)
    : SquadTimer(p, q)
{
}


// FUNCTION: 0x408620
void BuildTimer::TryIssueIdleFactoryBuildOrder(Unit* unit)
{
    if (unit->list != nullptr)
        return;
    unsigned short idx = ChooseBuildOption(player, unit);
    if (idx != 0)
        QueueBuildOrder((char*)&g_game->defs[idx].unitname[0], unit, 1);
}

// FUNCTION: 0x408670
void __stdcall UpdateConverter(Unit* unit)
{
    Economy* economy = (Economy*)unit->player.get();
    if (economy->cost + economy->cost < economy->energy) {
        if (g_seams.players->GetNetEnergy((Player*)economy) > Zero_004fc968 && RandomInt(5) != 0) {
            g_seams.units->SetStateBits(unit, 1, 1);
        }
    } else {
        g_seams.units->SetStateBits(unit, 1, 0);
    }
}

// FUNCTION: 0x4086d0
void BuildTimer::OnTimer()
{
    next = (unsigned int)g_game->ticks + 30;
    for (SimVector<Unit*>::iterator it = group->units.begin(); it != group->units.end(); ++it) {
        Unit* u = *it;
        if ((u->flags & 0x20000000) && (u->flags & 0x10000000) && !(u->flags & 0x4000)) {
            if (u->def->makesMetal) {
                if (((Economy*)u->player.get())->cost + ((Economy*)u->player.get())->cost < ((Economy*)u->player.get())->energy) {
                    if (g_seams.players->GetNetEnergy(u->player) > Zero_004fc968 && RandomInt(5))
                        g_seams.units->SetStateBits(u, 1, 1);
                } else
                    g_seams.units->SetStateBits(u, 1, 0);
            } else if (u->def->count && !u->list) {
                unsigned short id = ChooseBuildOption(player, u);
                if (id)
                    QueueBuildOrder((char*)&g_game->defs[id].unitname[0], u, 1);
            }
        }
    }
}

// FUNCTION: 0x4087e0
// FUNCTION: 0x408810 ??_GBuildTimer@@UAEPAXI@Z
BuildTimer::BuildTimer(SquadManager* p, Group* q)
    : SquadTimer(p, q)
{
}

// FUNCTION: 0x408830
void SquadManager::AssignSquads()
{
    for (Unit* u = player->unitsBegin; u <= player->unitsEnd; ++u) {
        if(u->flags&0x20) {
            if(((Flags245*)&u->def->flags2)->special) u->flags=(u->flags&~0x80000)|0x40000;
            else u->flags=(u->flags&~0x40000)|0x80000;
            u->flags=(u->flags&~0x100000)|0x200000;
            if(!u->group) {
                if(u->flags&0x20000000) {
                    if(u->flags&0x80000000) SetUnitSquad(u,5);
                    else SetUnitSquad(u,1);
                } else if(((Flags241*)&u->def->flags1)->builder) SetUnitSquad(u,4);
                else if(((Flags241*)&u->def->flags1)->flying) SetUnitSquad(u,8);
                else if(u->def->minwaterdepth>0) SetUnitSquad(u,7);
                else if(u->flags&0x80000000) SetUnitSquad(u,3);
            }
        }
    }
}

// FUNCTION: 0x408920
void __stdcall RetargetWeapon(Unit* unit, unsigned int weapon)
{
    if (unit->weapons[weapon].weapon->flags.b.interceptor) {
        Projectile* p = g_seams.weapons->FindTargetableProjectile(unit, weapon);
        if (p)
            g_seams.targeting->SetWeaponTargetPos(unit, (Vec3*)((int*)p + 1), weapon);
        else
            g_seams.targeting->ClearWeaponTarget(unit, weapon);
    } else if ((unit->flags & 0x300000) == 0x200000) {
        Unit* r = g_seams.targeting->FindWeaponTarget(unit, weapon, 1);
        if (r)
            g_seams.targeting->SetWeaponTargetUnit(unit, r, weapon);
        else
            g_seams.targeting->ClearWeaponTarget(unit, weapon);
    }
}

// FUNCTION: 0x4089a0
void SquadManager::RetargetWeapons(int force)
{
    for (int i = 0; i <= g_game->maxUnitsSetting / 30; i++) {
        if (cursor && cursor != player->unitsEnd) {
            Unit* next = cursor;
            next++;
            cursor = next;
        } else
            cursor = player->unitsBegin;
        if (cursor->unitDefIndex != 0 && cursor->buildLeft == Zero_004fc968 && (cursor->flags & 0x80000000)
            && (cursor->flags & 0x300000) == 0x200000) {
            for (unsigned char w = 0; w < 3; w++) {
                if ((cursor->weapons[w].flags & 2) && (cursor->weapons[w].flags & 0x10)
                    && !(unsigned char)cursor->weapons[w].weapon->flags.b.dropped
                    && (force || !cursor->weapons[w].weapon->flags.b.commandFire)) {
                    Unit* target = g_seams.targeting->GetWeaponTargetUnit(cursor, w);
                    if (target && (player->allied[target->player->index]
                        || Contains(cursor->def->weaponCategories[w], target->unitDefIndex)
                        || (cursor->weapons[w].weapon->flags.b.paralyzer && (target->activateFlags & 0x10))))
                        target = 0;
                    if (!target)
                        RetargetWeapon(cursor, w);
                }
            }
        }
    }
}

// FUNCTION: 0x408bf0
void SquadManager::TickTimers()
{
    if (--countdown <= 0) {
        countdown = 30;
        AssignSquads();
    }
    for (int i = 0; i < 10; i++) {
        if (timers[i] != 0 && timers[i]->next <= (unsigned int)g_game->ticks) {
            timers[i]->OnTimer();
        }
    }
}

// FUNCTION: 0x408c40
void SquadManager::TickIfActive()
{
    if (player->active != 0 && player->type == 2) {
        if (--countdown <= 0) {
            countdown = 30;
            AssignSquads();
        }
        for (int i = 0; i < 10; i++) {
            if (timers[i] != 0 && timers[i]->next <= (unsigned int)g_game->ticks) {
                timers[i]->OnTimer();
            }
        }
        RetargetWeapons(1);
        return;
    }
    RetargetWeapons(0);
}

// FUNCTION: 0x408cb0
SquadManager::SquadManager(Player* p)
{
    player = p;
    index = p->index;
    unused_9 = 0;
    countdown = 30;
    cursor = 0;
    nextAction = 0;
    for (int i = 0; i < 10; i++)
        timers[i] = 0;
    timers[1] = new (kSim) BuildTimer(this, &player->groups[1]);
    timers[4] = new (kSim) ScoutTimer(this, &player->groups[4]);
    timers[5] = new (kSim) SquadTimer(this, &player->groups[5]);
    timers[2] = new (kSim) AssaultTimer(this, &player->groups[2], 3, 20000);
    timers[3] = new (kSim) EscortTimer(this, &player->groups[3], 2);
    timers[6] = new (kSim) AssaultTimer(this, &player->groups[6], 7, 50000);
    timers[7] = new (kSim) EscortTimer(this, &player->groups[7], 6);
    timers[8] = new (kSim) SquadScoutTimer(this, &player->groups[8]);
    timers[9] = new (kSim) SpatialTimer(this, &player->groups[9]);
}

// FUNCTION: 0x408f10
void SquadManager::DeleteTimers()
{
    for (int i = 0; i < 10; i++) {
        if (timers[i] != 0) {
            delete timers[i];
        }
    }
}

typedef SimVector<Unit*> Vec_00408f30;
typedef void (Vec_00408f30::*InsertFn_00408f30)(
    Vec_00408f30::iterator, Vec_00408f30::size_type, Unit* const&);

void __stdcall Grow_00408f30(Vec_00408f30* units, int extra)
{
    units->reserve(extra + units->size());
}

// FUNCTION: 0x408f30 ?insert@?$vector@PAUUnit@@V?$allocator@PAUUnit@@@std@@@std@@QAEXPAPAUUnit@@IABQAU3@@Z
InsertFn_00408f30 g_insert_00408f30 = &Vec_00408f30::insert;

// FUNCTION: 0x409160
PlayerAI::PlayerAI(unsigned char p)
    : owner(&g_game->players[p]), index(p),
      center(g_game->width / 2, g_game->height / 2), builders(0)
{
    lastTick = 0;
    searchRadius = 0;
    InitPlacementGrid();
    int n = g_game->unitDefCount;
    weights.resize(n, 0);
    counts.resize(n, 0);
    {
        Elem_0040cfb0 e;
        e.a = 0;
        e.b = 0;
        e.c = 0;
        vec_65.resize(n, e);
    }
    {
        Elem_0040d4f0 f;
        f.value = 0;
        vec_ad.resize(n, f);
    }
    {
        Elem_0040d550 g;
        g.unknown_0 = 0;
        vec_bd.resize(n, g);
        g.unknown_0 = 0;
        values.resize(n, g);
        g.unknown_0 = 0;
        locked.resize(n, g);
    }
    InitUnitTables();
}

// FUNCTION: 0x409470
void PlayerAI::InitUnitTables()
{
    int n = g_game->unitDefCount;
    for (int i = 0; i < n; ++i) {
        UnitDef* def = &g_game->defs[i];
        weights[i] = 0;
        if (!def->mobile)
            weights[i] += 40;
        if (def->ids)
            weights[i] += 20;
        counts[i] = 0;
        vec_ad[i].value = 100;
        vec_bd[i].unknown_0 = 0;
        values[i].unknown_0 = -1;
        locked[i].unknown_0 = 0;
    }
}

#define MIN(a, b) (((a) > (b)) ? (b) : (a))

// FUNCTION: 0x409520
int __stdcall RateWeapons(UnitDef* p)
{
    int result = 1;
    if (((Flags245*)&p->flags2)->flag4)
        result = 0xb;
    Ref<Sub_00409520>* pp = (Ref<Sub_00409520>*)p->weapons;
    for (int i = 3; i != 0; i--) {
        Sub_00409520* s = pp->get();
        if (s->index != 0)
            result = result + s->damage / 40 + s->range / 100 + 5;
        pp++;
    }
    if (MIN(result, 100) < -100)
        return -100;
    return MIN(result, 100);
}

// FUNCTION: 0x4095d0
int __stdcall RateUnitType(UnitDef* p)
{
    int result = 1;
    if (p->extractsMetal != 0.0f)
        result = 0xb;
    if (p->makesMetal != 0)
        result += 10;
    if (g_seams.economy->GetEnergyUse(p) < 0.0f)
        result += 10;
    result = (int)((int)(result - p->metalCost * -0.01f) - p->energyCost * -0.002f);
    int extra = 1;
    if (((Flags245*)&p->flags2)->flag4)
        extra = 0xb;
    Ref<Sub_00409520>* pp = (Ref<Sub_00409520>*)p->weapons;
    for (int i = 3; i != 0; i--) {
        Sub_00409520* s = pp->get();
        if (s->index != 0)
            extra = extra + s->damage / 40 + s->range / 100 + 5;
        pp++;
    }
    result += (signed char)((MIN(extra, 100) < -100) ? -100 : MIN(extra, 100));
    if (MIN(result, 100) < -100)
        return -100;
    return MIN(result, 100);
}

int __stdcall RateUnitTypeForWeights(UnitDef* p)
{
    int result = 1;
    if (p->extractsMetal != 0.0f)
        result = 0xb;
    if (p->makesMetal != 0)
        result += 10;
    if (g_seams.economy->GetEnergyUse(p) < 0.0f)
        result += 10;
#ifdef __clang__
    result = (int)((int)((double)result - (double)p->metalCost * -0.01f) - (double)p->energyCost * -0.002f);
#else
    result = (int)((int)(result - p->metalCost * -0.01f) - p->energyCost * -0.002f);
#endif
    result += (signed char)RateWeapons(p);
    if (MIN(result, 100) < -100)
        return -100;
    return MIN(result, 100);
}

// FUNCTION: 0x409730
void PlayerAI::ComputeBaseWeights()
{
    vec_8d.resize(g_game->unitDefCount, 0);
    {
        Elem_0040cfb0 e;
        e.a = 0;
        e.b = 0;
        e.c = 0;
        vec_65.resize(g_game->unitDefCount, e);
    }
    for (int i = 1; i < g_game->unitDefCount; i++) {
        UnitDef* def = &g_game->defs[(unsigned short)i];
        vec_8d[i] = RateUnitTypeForWeights(def);

        int a = 1;
        Elem_0040cfb0* e = &vec_65[i];
        int n = (short)counts[i];
        if (def->c04)
            a = 21;
        if (def->canbuild && n < 3)
            a += 30;
        if (g_seams.economy->GetEnergyUse(def) < 0.0f)
            a += 50;
        if (def->HasField1ce())
            a += 50;
        if (def->makesMetal)
            a += 25;
        unsigned int flags = def->flags1;
        if ((flags >> 11) & 1)
            a += 40;
        if (def->sonardistance)
            a += 15;
        if (def->radardistance)
            a += 5;
        int x = (int)(a + min(max(def->energymake, 0.0f), 30.0f));
        if (n == 0)
            x *= 4;
        if (n == 1)
            x *= 2;
        if (def->minwaterdepth >= 0)
            x *= 3;
        if (owner->unitCount > (unsigned short)(g_game->unitsPerPlayer / 2)) {
            x += (char)vec_8d[i] / 2;
        }
        if (def->c08)
            x = 0;
        if ((flags >> 24) & 1)
            x = 0;
        if (def->windgenerator != 0.0f && g_game->windSpeedMax < g_game->windSpeedDenominator / 2)
            x = 0;
        x = min(x, 100);
        e->a = x;
        e->c = (char)max(0.0f, min(100.0f, def->energyCost * -0.0025f - g_seams.economy->GetEnergyUse(def) * 5.0f));
#ifdef __clang__
        e->b = (char)max(0.0f, min(100.0f, (double)(float)((double)def->metalCost * -0.02f + (def->makesMetal ? 25 : 0)) + (def->extractsMetal != 0.0f ? 100 : 0)));
#else
        e->b = (char)max(0.0f, min(100.0f, (float)(def->metalCost * -0.02f) + (def->makesMetal ? 25 : 0) + (def->extractsMetal != 0.0f ? 100 : 0)));
#endif
    }
}

// FUNCTION: 0x409dc0
void __stdcall ScaleUnitWeights(int player, unsigned int* mask, float scale, int lock)
{
    PlayerAI* p = g_playerAI[player];
    for (unsigned short i = 1; i < g_game->unitDefCount; i++) {
        if (mask[i >> 5] & (1 << (i & 0x1f))) {
            int off = i * 4;
            if (*(int*)(off + (int)p->vec_bd.begin()) == 0) {
                p->vec_ad[i].value = (unsigned char)__min(__max((int)(p->vec_ad[i].value * scale), 0), 100);
                if (lock)
                    *(int*)(off + (int)p->vec_bd.begin()) = 1;
            }
        }
    }
}

// FUNCTION: 0x409e90
void __stdcall SetUnitLimits(int player, unsigned int* mask, int value, int lock)
{
    PlayerAI* p = g_playerAI[player];
    for (unsigned short i = 1; i < g_game->unitDefCount; i++) {
        if ((mask[i >> 5] & (1 << (i & 0x1f))) && p->locked[i].unknown_0 == 0) {
            p->values[i].unknown_0 = value;
            if (lock)
                p->locked[i].unknown_0 = 1;
        }
    }
}

#pragma auto_inline(off)
// FUNCTION: 0x409f20
int __stdcall IsUnderLimit(int player, unsigned short index, int value)
{
    if (index >= 1 && index < g_game->unitDefCount) {
        int* values = (int*)&g_playerAI[player]->values[0];
        if (values[index] == -1)
            return 1;
        return value < values[index];
    }
    return 0;
}
#pragma auto_inline(on)

// FUNCTION: 0x409f80
void __stdcall ParseDownloadableAiWeightScripts(int player)
{
    PlayerAI* p = g_playerAI[player];
    EnableAICommands();
    for (unsigned short i = 1; i < g_game->unitDefCount; i++) {
        UnitDef* def = &g_game->defs[i];
        if (((Flags241*)&def->flags1)->downloadable) {
            if (p->vec_bd[i].unknown_0 != 1) {
                int len = strlen(def->aiWeight);
                if (len != 0) {
                    CommandArgs vars;
                    vars.InitArgs();
                    ExecuteCommandText(def->aiWeight, len, &vars, -1);
                }
            }
        }
    }
}

// FUNCTION: 0x40a040
void __stdcall ReparseAiWeightScriptsIfLimitNotSticky(int player)
{
    PlayerAI* p = g_playerAI[player];
    EnableAICommands();
    for (unsigned short i = 1; i < g_game->unitDefCount; i++) {
        UnitDef* def = &g_game->defs[i];
        if (((Flags241*)&def->flags1)->downloadable) {
            if (p->locked[i].unknown_0 != 1) {
                int len = strlen(def->aiWeight);
                if (len != 0) {
                    CommandArgs vars;
                    vars.InitArgs();
                    ExecuteCommandText(def->aiWeight, len, &vars, -1);
                }
            }
        }
    }
}

// FUNCTION: 0x40a100
void ResetAIPlayers()
{
    for (int i = 0; i < 10; i++) {
        if (g_game->players[i].active != 0 && g_game->players[i].type == 2) {
            g_playerAI[i]->InitUnitTables();
        }
    }
    g_seams.players->LoadDefaultAIScript();
}

// FUNCTION: 0x40a150
void PlayerAI::InitPlacementGrid()
{
    margin0 = 3;
    spacing0.x = RandomInt(10) + margin0 + 8;
    spacing0.y = RandomInt(3) + margin0 + 8;
    offset0.x = RandomInt(spacing0.x) - spacing0.x / 2;
    offset0.y = RandomInt(spacing0.y) - spacing0.y / 2;
    margin1 = 6;
    spacing1.x = RandomInt(0x14) + margin1 + 8;
    spacing1.y = RandomInt(3) + margin1 + 8;
    offset1.x = RandomInt(spacing1.x) - spacing1.x / 2;
    offset1.y = RandomInt(spacing1.y) - spacing1.y / 2;
}

// FUNCTION: 0x40a260
bool PlayerAI::FindCellNearFeatures(UnitDef* type, Vec3* pos, SimVector<Elem_0040cc40>* list, int range, Point16* out)
{
    if (list->empty())
        return false;
    std::vector<Elem_0040cc40> heap;
    heap.reserve(list->size());
    int rangeSq = range * range;
    Point16 center = WorldToCell(*pos, *(Point16*)&type->f14a);
    for (SimVector<Elem_0040cc40>::iterator p = list->begin(); p != list->end(); p++) {
        int d = DistSq(p->pos, center);
        if (d <= rangeSq) {
            heap.push_back(*p);
            heap.back().key = -d;
        }
    }
    if (2 <= heap.end() - heap.begin())
        MakeHeap((Entry_40d670 *)heap.begin(), (Entry_40d670 *)heap.end(), (int*)0, (Entry_40d670 *)(Elem_0040cc40*)0);
    int limit = -1;
    int best = 0;
    Point16 result;
    while (!heap.empty()) {
        Point16 cell = heap.front().pos;
        cell.x -= (type->footprintx - 3) / 2;
        cell.y -= (type->footprintz - 3) / 2;
        int d = DistSq(cell, center);
        if (limit >= 0 && d > limit + 160)
            break;
        if (g_seams.map->CanBuildAt(type, cell, 0, 0) && GetBuildSiteMetal() > best) {
            result = cell;
            best = GetBuildSiteMetal();
            if (limit == -1)
                limit = d;
        }
        PopHeap(heap.begin(), heap.end());
        heap.pop_back();
    }
    if (best == 0)
        return false;
    if (out)
        *out = result;
    return true;
}

// FUNCTION: 0x40a5d0
bool PlayerAI::FindRandomPlacementCell(UnitDef* type, Vec3* pos, int range, Point16* out)
{
    int areaY = type->footprintz;
    int areaX = type->footprintx;
    int threshold = g_game->net->surfaceMetal * areaY * areaX * 2;
    Point16 spacing = type->minwaterdepth < 0 ? spacing0 : spacing1;
    Point16 offset = type->minwaterdepth < 0 ? offset0 : offset1;
    int margin = type->minwaterdepth < 0 ? margin0 : margin1;
    for (int i = 0; i < 30; i++) {
        int dist = RandomInt(range) << 16;
        int angle = RandomInt(0x10000);
        Vec3 v = PosPlus(Direction(angle, dist), *pos);
        Point16 cell = WorldToCell(v, *(Point16*)&type->f14a);
        cell.x = cell.x / spacing.x * spacing.x + offset.x + RandomInt(spacing.x - margin - type->footprintx);
        cell.y = cell.y / spacing.y * spacing.y + offset.y + RandomInt(spacing.y - margin - type->footprintz);
        if (g_seams.map->CanPlaceUnitFootprint(type, 0, cell, 1) && GetBuildSiteMetal() <= threshold) {
            if (out)
                *out = cell;
            return true;
        }
    }
    return false;
}

// FUNCTION: 0x40a7b0
void PlayerAI::BuildFeatureCells()
{
    cells.clear();
    int w = g_game->width;
    for (int y = 0; y < g_game->height; y++) {
        Cell* row = g_seams.los_radar->GetMapCell(0, y);
        for (int x = 0; x < w; x++) {
            if (row[x].feature < 0xfffb) {
                Feature* f = &g_game->features[row[x].feature];
                if (f->value != 0.0f && (f->flags & 0x200))
                    cells.push_back(Elem_0040cc40(x, y, f->value));
            }
        }
    }
}

// FUNCTION: 0x40ad20
void PlayerAI::UpdateEveryThirtyTicks()
{
    if ((unsigned int)g_game->ticks >= lastTick + 0x1e) {
        this->RefreshUnitLists();
        lastTick = (unsigned int)g_game->ticks;
        if (RandomInt(0x1e) == 0) {
            this->ComputeBaseWeights();
        }
    }
}
// FUNCTION: 0x40ad70
void __stdcall ProbeUnitDefEnergyRate(int unused1, int unused2, int param_3)
{
    g_seams.economy->GetEnergyUse((UnitDef*)param_3);
}

// FUNCTION: 0x40ad80
void __stdcall GetVisibleEnemiesInRadius(int player,const Vec* pos,int radius,int flags,std::vector<Unit*>* out)
{
    int radius2=radius*radius;
    SimVector<Unit*>& visible=g_playerAI[player]->visible.units;
    SimVector<Unit*>::iterator it;
    for(it=visible.begin();it!=visible.end();++it) {
        Unit* unit=*it;
        int dz=pos->z-unit->pos.z;
        int dx=pos->x-unit->pos.x;
        int d=(int)(((__int64)dx*dx)>>32)+(int)(((__int64)dz*dz)>>32);
        if(d<=radius2 && unit->Ready()) out->push_back(unit);
    }
    PlayerAI* owner=g_playerAI[player];
    if(owner->hasSpecial && out->empty()) {
        for(it=owner->known.units.begin();it!=owner->known.units.end();++it) {
            Unit* unit=*it;
            int dz=pos->z-unit->pos.z;
        int dx=pos->x-unit->pos.x;
        int d=(int)(((__int64)dx*dx)>>32)+(int)(((__int64)dz*dz)>>32);
        if(d<=radius2 && unit->Ready()) out->push_back(unit);
        }
    }
}

// FUNCTION: 0x40b0d0
bool __stdcall HasReadyUnitInRange(int player, Vec3_40b0d0* p, int range)
{
    int r2 = range * range;
    PlayerAI* t = g_playerAI[player];
    SimVector<Unit*>& units = t->visible.units;
    SimVector<Unit*>::iterator it = units.begin();
    if (it != units.end()) {
        do {
            Unit* u = *it;
            int dz = p->z - u->pos.z;
            int dx = p->x - u->pos.x;
            if ((int)(((__int64)dx * dx) >> 32) + (int)(((__int64)dz * dz) >> 32) <= r2
                && (u->flags & 0x10000000)
                && !(u->flags & 0x4000))
                return true;
        } while (++it != units.end());
    }
    return false;
}

// FUNCTION: 0x40b1c0
int __stdcall SumUnitRatingsInRange(int player, Vec3* pos, int range)
{
    int total = 0;
    PlayerAI* p = g_playerAI[player];
    int r2 = range * range;
    SimVector<Unit*>& units = p->visible.units;
    for (SimVector<Unit*>::iterator it = units.begin(); it != units.end(); it++) {
        Unit* u = *it;
        int dz = pos->z - u->pos.z;
        int dx = pos->x - u->pos.x;
        int d = (int)(((__int64)dx * dx) >> 32) + (int)(((__int64)dz * dz) >> 32);
        if (d <= r2)
            total += (signed char)p->vec_8d[u->unitDefIndex];
    }
    return total;
}

// FUNCTION: 0x40b2c0
void __stdcall UpdatePlayerAI(int player)
{
    PlayerAI* p = g_playerAI[player];
    if (p && (unsigned int)g_game->ticks >= p->lastTick + 0x1e) {
        p->RefreshUnitLists();
        p->lastTick = (unsigned int)g_game->ticks;
        if (RandomInt(0x1e) == 0) {
            p->ComputeBaseWeights();
        }
    }
}

// FUNCTION: 0x40b320
void __stdcall CreatePlayerAI(int player)
{
    Ref<PlayerAI>& slot = g_playerAI[player];
    slot = new (kSim) PlayerAI(player);
    slot->ComputeBaseWeights();
}

// FUNCTION: 0x40b370
void __stdcall RebuildFeatureCells(int param_1)
{
    g_playerAI[param_1]->BuildFeatureCells();
}

// FUNCTION: 0x40b390
void __stdcall DestroyPlayerAI(int player)
{
    delete g_playerAI[player].get();
    g_playerAI[player]=0;
}

namespace std {
inline void _Construct(Unit** dest, Unit* const& src) { CopyDwordIfNonNull((int *)dest, (int *)&src); }
}

// FUNCTION: 0x40b530
void __stdcall GetFactoriesInRadius(int player,const Vec* pos,int radius,std::vector<Unit*>* out)
{
    SimVector<Unit*>& list=g_playerAI[player]->factories.list.units;
    radius*=radius;
    for(SimVector<Unit*>::iterator it=list.begin();it!=list.end();++it) {
        Unit* unit=*it;
        if((unit->def->flags1&0x40) && (unit->def->flags1&0x200) && (unit->activateFlags&1)) {
            int dz=pos->z-unit->pos.z;
            int dx=pos->x-unit->pos.x;
            int d=(int)(((__int64)dx*dx)>>32)+(int)(((__int64)dz*dz)>>32);
            if(d<=radius) out->push_back(*it);
        }
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
    if(useRange) g_seams.ai->GetVisibleEnemiesInRadius(unit->playerIndex,(const Vec3*)&unit->pos,g_seams.weapons->GetWeaponRange(unit,weapon),0,&candidates);
    else g_seams.ai->GetVisibleEnemiesInRadius(unit->playerIndex,(const Vec3*)&unit->pos,unit->def->range,0,&candidates);
    for(int count=0;count<50;++count) {
        if(candidates.empty()) break;
        std::vector<Unit*>::iterator it=candidates.begin()+RandomInt(candidates.size());
        Unit* target=*it;
        ((Class_004800c0*)&candidates)->EraseSwapBack(it);
        if((target->flags&0x10000000) && !(target->flags&0x4000) &&
           ((target->def->flags1&0x8000) || ai || (g_game->matchFlags&4)) &&
           ((unit->def->flags1&0x10000000) || g_seams.weapons->WeaponCanReachUnit(unit,target,weapon)) &&
           (useRange || !((Bits*)&unit->def->categories)->Test(target->unitDefIndex)) &&
           (!((*(unsigned char*)&unit->weapons[weapon].weapon->flags)&0x80) || !(target->activateFlags&0x10))) {
            int dz=unit->pos.z-target->pos.z;
            int dx=unit->pos.x-target->pos.x;
            int d=RandomInt((int)(((__int64)dx*dx)>>32)+(int)(((__int64)dz*dz)>>32));
            if(((Bits*)unit->def->weaponCategories)[weapon].Test(target->unitDefIndex)) {
                if(d<fallbackDistance) { fallbackDistance=d; fallback=target; }
            } else if(d<bestDistance) { bestDistance=d; best=target; }
        }
    }
    if(best) return best;
    return fallback;
}

// FUNCTION: 0x40ba80
void __stdcall GetBasePosition(int index, Struct_0040ba80* out)
{
    Struct_0040ba80* entry = (Struct_0040ba80*)((char*)g_playerAI[index].get() + 0x35);
    *out = *entry;
}

// FUNCTION: 0x40bab0
void __stdcall GetMirroredBasePosition(int index, int unused, Struct_0040bab0* out)
{
    out->a = (g_game->baseX << 16) -
             ((Struct_0040bab0*)((char*)g_playerAI[index].get() + 0x35))->a;
    out->b = 0;
    out->c = (g_game->baseX << 16) -
             ((Struct_0040bab0*)((char*)g_playerAI[index].get() + 0x35))->c;
}

// FUNCTION: 0x40bb00
int __stdcall GetBuildRating(int player,unsigned short type)
{
#pragma pack(push, 1)
    struct Owner_40bb00 {
        char pad[0x69];
        Elem_0040cfb0* ratings;
        char pad6d[0x81-0x6d];
        short* counts;
        char pad85[0xb1-0x85];
        unsigned char* weights;
    };
#pragma pack(pop)
    Owner_40bb00* owner=(Owner_40bb00*)g_playerAI[player].get();
    Player* p=&g_game->players[player];
    if(p->energy<50.0f) return 0;
    if(p->metal<25.0f) return 0;
    if(g_seams.map->GetGameType(g_game->net)==1 && (g_game->defs[type].flags1&0x20)) return 0;
    int energyCap=min(1000,(int)p->energyCapacity);
    int metalCap=min(500,(int)p->metalCapacity);
    int energy=(int)Max(0.0f,(energyCap-p->energy)*0.125f);
    int metal=(int)Max(0.0f,(metalCap-p->metal)*0.25f);
    if(g_seams.players->GetNetEnergy(p)<1.0f) energy+=20;
    if(g_seams.players->GetNetMetal(p)<1.0f) metal+=20;
    if(g_seams.players->GetEnergyIncome(p)<50.0f) energy+=100;
    else if(g_seams.players->GetEnergyIncome(p)<200.0f) energy+=10;
    if(g_seams.players->GetMetalIncome(p)<3.0f) metal+=100;
    else if(g_seams.players->GetMetalIncome(p)<5.0f) metal+=20;
    int metal2=min(max(metal,0),100);
    int energy2=min(max(energy-metal2,0),100);
    int normal=max(100-metal2-energy2,0);
    if(!IsCountBelowAiLimit(player,type,owner->counts[type])) return 0;
    Elem_0040cfb0* r=&owner->ratings[type];
    return (normal*r->a+r->b*metal2+r->c*energy2)*owner->weights[type]/10000;
}

// FUNCTION: 0x40bdb0
unsigned short __stdcall ChooseBuildOption(unsigned int player, Unit* unit)
{
    unsigned short chosen = 0;
    int total = 0;
    for (int i = 0; i < unit->def->count; i++) {
        unsigned short id = unit->def->ids[i];
        int r = GetBuildRating(player, id);
        if (r > 0) {
            total += r;
            if (RandomInt(total) < r)
                chosen = id;
        }
    }
    if (chosen != 0) {
        if (strcmp(unit->def->side, g_game->defs[chosen].side) != 0)
            chosen = 0;
    }
    return chosen;
}

// FUNCTION: 0x40beb0
Vec3_0040beb0 __stdcall MoveToward(const Vec3_0040beb0* from, const Vec3_0040beb0* to,
                                     int maxLen)
{
    Vec3_0040beb0 d = *to - *from;
    int len = d.Length();
    if (maxLen >= len)
        return *to;
    d.Scale((int)(((__int64)maxLen << 16) / len));
    Vec3_0040beb0 r;
    r.x = from->x + d.x;
    r.y = from->y + d.y;
    r.z = from->z + d.z;
    return r;
}

// FUNCTION: 0x40bfe0
int __stdcall FindBuildPosition(int player, const Vec3* from, UnitDef* type, Vec3* out)
{
    PlayerAI* ai=g_playerAI[player];
    int maximum=g_game->baseX > g_game->baseY ? g_game->baseX : g_game->baseY;
    if (ai->searchRadius<maximum) ai->searchRadius+=160;
    Vec3 pos=MoveTowards(from,(const Vec3*)&ai->centre,ai->searchRadius<<16);
    Point16 cell;
    int result;
    if (type->extractsMetal!=0.0f && g_game->net->surfaceMetal<RandomInt(255))
        result=ai->FindCellNearFeatures(type,&pos,&ai->cells,ai->searchRadius*4,&cell);
    else result=ai->FindRandomPlacementCell(type,&pos,ai->searchRadius,&cell);
    if (result) {
        CellToWorld(out,cell,*(Point16*)&type->f14a);
        ai->searchRadius=0;
    }
    return result;
}

// FUNCTION: 0x40c200
int __stdcall GetUnitCount(int param_1, unsigned int param_2)
{
    unsigned int idx = param_2 & 0xffff;
    void* p = g_playerAI[param_1];
    int* ptr = (int*)((char*)p + 0x81);
    short* arr = (short*)(*ptr);
    return (int)arr[idx];
}

// FUNCTION: 0x40c230
int __stdcall GetBuilderCount(int param_1)
{
    void* ptr = g_playerAI[param_1].get();
    return *(int*)((char*)ptr + 0x75);
}

// FUNCTION: 0x40c250
void __stdcall DumpPlayerAI(int player, _iobuf* file)
{
    PlayerAI* ai=g_playerAI[player];
    char buffer[256];
    unsigned int hours=(unsigned int)g_game->ticks/108000;
    int remaining=(unsigned int)g_game->ticks-hours*108000;
    int minutes=remaining/1800;
    int seconds=(remaining-minutes*1800)/30;
    sprintf(buffer,"%02d:%02d:%02d",hours,minutes,seconds);
    FilePrintf(file,"gametime: '%s'\r\n",buffer);
    FilePrintf(file,"player:   '%s' num: %d\r\n",g_game->players[player].name,player);
    switch(g_game->players[player].type) {
    case 1: strcpy(buffer,"HUMAN"); break;
    case 2: strcpy(buffer,"AI"); break;
    default: strcpy(buffer,"INVALID"); break;
    }
    FilePrintf(file,"controller: %s\r\n",buffer);
    FilePrintf(file,"terrain:    '%s'\r\n",g_seams.map->GetNameSlot(g_game->net,1));
    FilePrintf(file,"profile:    '%s'\r\n",g_seams.map->GetNameSlot(g_game->net,7));
    const char* difficulties[]={"EASY","MEDIUM","HARD"};
    FilePrintf(file,"difficulty: '%s'\r\n",difficulties[g_game->difficulty]);
    FilePrintf(file,"================================================\r\n");
    FilePrintf(file,"<limit> - <base:baseML:baseEL> : <end result - before economy-based tweaks> - <unit name>\r\n");
    for (unsigned short i=1;i<g_game->unitDefCount;++i) {
        UnitDef* type=&g_game->defs[i];
        if (ai->values[i].unknown_0<0) FilePrintf(file,"n/a ");
        else FilePrintf(file,"%4d",ai->values[i].unknown_0);
        sprintf(buffer," - %3d : %3d : %3d = %3d - '%s\t\t:%s'\r\n",ai->vec_65[i].a,ai->vec_65[i].b,ai->vec_65[i].c,ai->vec_ad[i].value,type->unitname,type->name);
        FilePrintf(file,buffer);
    }
}

// FUNCTION: 0x40c4f0
int __stdcall HasSpecialUnit(int param_1)
{
    return *(int*)((char*)g_playerAI[param_1].get() + 0x79);
}

template class std::vector<Unit*, SimAllocator<Unit*> >;

// FUNCTION: 0x40c510 ??0?$vector@PAUUnit@@V?$allocator@PAUUnit@@@std@@@std@@QAE@ABV?$allocator@PAUUnit@@@1@@Z

inline void __stdcall CopyOneByte(Elem_0040d4f0* dest, const Elem_0040d4f0* src) { new (dest) Elem_0040d4f0(*src); }
inline void __stdcall CopyDwordPair(Elem_0040cc40* dest, const Elem_0040cc40* src) { new (dest) Elem_0040cc40(*src); }
namespace std {
inline void _Construct(Elem_0040d4f0* dest, const Elem_0040d4f0& src) { CopyOneByte(dest, &src); }
inline void _Construct(Elem_0040cc40* dest, const Elem_0040cc40& src) { CopyDwordPair(dest, &src); }
}

typedef SimVector<Unit*> Vec_0040c530;
typedef SimVector<Elem_0040cc40> Vec_0040c580;
typedef SimVector<Elem_0040cfb0> Vec_0040c5d0;
typedef SimVector<Elem_0040d4f0> Vec_0040c600;
typedef SimVector<Elem_0040d550> Vec_0040c7f0;
typedef SimVector<short> Vec_0040d000;
typedef SimVector<unsigned char> Vec_0040d290;

typedef SimVector<Vec_0040c530> Outer_0040c530;
typedef Outer_0040c530& (Outer_0040c530::*AssignFn_0040c530)(const Outer_0040c530&);
// FUNCTION: 0x40c530 ??1?$vector@PAUUnit@@V?$allocator@PAUUnit@@@std@@@std@@QAE@XZ
AssignFn_0040c530 g_assign_0040c530 = &Outer_0040c530::operator=;

typedef Vec_0040c530::size_type (Vec_0040c530::*SizeFn_0040c560)() const;
// FUNCTION: 0x40c560 ?size@?$vector@PAUUnit@@V?$allocator@PAUUnit@@@std@@@std@@QBEIXZ
SizeFn_0040c560 g_size_0040c560 = &Vec_0040c530::size;

typedef SimVector<Vec_0040c580> Outer_0040c580;
typedef Outer_0040c580& (Outer_0040c580::*AssignFn_0040c580)(const Outer_0040c580&);
// FUNCTION: 0x40c580 ??1?$vector@UElem_0040cc40@@V?$allocator@UElem_0040cc40@@@std@@@std@@QAE@XZ
AssignFn_0040c580 g_assign_0040c580 = &Outer_0040c580::operator=;

typedef Vec_0040c580::size_type (Vec_0040c580::*SizeFn_0040c5b0)() const;
// FUNCTION: 0x40c5b0 ?size@?$vector@UElem_0040cc40@@V?$allocator@UElem_0040cc40@@@std@@@std@@QBEIXZ
SizeFn_0040c5b0 g_size_0040c5b0 = &Vec_0040c580::size;

typedef SimVector<Vec_0040c5d0> Outer_0040c5d0;
typedef Outer_0040c5d0& (Outer_0040c5d0::*AssignFn_0040c5d0)(const Outer_0040c5d0&);
// FUNCTION: 0x40c5d0 ??1?$vector@UElem_0040cfb0@@V?$allocator@UElem_0040cfb0@@@std@@@std@@QAE@XZ
AssignFn_0040c5d0 g_assign_0040c5d0 = &Outer_0040c5d0::operator=;

typedef void (Vec_0040c600::*ResizeFn_0040c600)(unsigned int, const Elem_0040d4f0&);
// FUNCTION: 0x40c600 ?resize@?$vector@UElem_0040d4f0@@V?$allocator@UElem_0040d4f0@@@std@@@std@@QAEXIABUElem_0040d4f0@@@Z
ResizeFn_0040c600 g_resize_0040c600 = &Vec_0040c600::resize;

typedef void (Vec_0040c7f0::*ResizeFn_0040c7f0)(unsigned int, const Elem_0040d550&);
// FUNCTION: 0x40c7f0 ?resize@?$vector@UElem_0040d550@@V?$allocator@UElem_0040d550@@@std@@@std@@QAEXIABUElem_0040d550@@@Z
ResizeFn_0040c7f0 g_resize_0040c7f0 = &Vec_0040c7f0::resize;

typedef Vec_0040c530::iterator (Vec_0040c530::*EraseFn_0040c9f0)(
    Vec_0040c530::iterator, Vec_0040c530::iterator);
// FUNCTION: 0x40c9f0 ?erase@?$vector@PAUUnit@@V?$allocator@PAUUnit@@@std@@@std@@QAEPAPAUUnit@@PAPAU3@0@Z
EraseFn_0040c9f0 g_erase_0040c9f0 = &Vec_0040c530::erase;

typedef Vec_0040c580::size_type (Vec_0040c580::*CapacityFn_0040ca30)() const;
// FUNCTION: 0x40ca30 ?capacity@?$vector@UElem_0040cc40@@V?$allocator@UElem_0040cc40@@@std@@@std@@QBEIXZ
CapacityFn_0040ca30 g_capacity_0040ca30 = &Vec_0040c580::capacity;

typedef Vec_0040c580::iterator (Vec_0040c580::*InsertFn_0040ca50)(Vec_0040c580::iterator, const Elem_0040cc40&);
// FUNCTION: 0x40ca50 ?insert@?$vector@UElem_0040cc40@@V?$allocator@UElem_0040cc40@@@std@@@std@@QAEPAUElem_0040cc40@@PAU3@ABU3@@Z
InsertFn_0040ca50 g_insert_0040ca50 = &Vec_0040c580::insert;

typedef Vec_0040c5d0::size_type (Vec_0040c5d0::*SizeFn_0040cc80)() const;
// FUNCTION: 0x40cc80 ?size@?$vector@UElem_0040cfb0@@V?$allocator@UElem_0040cfb0@@@std@@@std@@QBEIXZ
SizeFn_0040cc80 g_size_0040cc80 = &Vec_0040c5d0::size;

typedef void (Vec_0040c5d0::*InsertFn_0040cca0)(Vec_0040c5d0::iterator, unsigned int, const Elem_0040cfb0&);

void __stdcall Grow_0040cca0(Vec_0040c5d0* v, int extra)
{
    v->reserve(extra + v->size());
}

// FUNCTION: 0x40cca0 ?insert@?$vector@UElem_0040cfb0@@V?$allocator@UElem_0040cfb0@@@std@@@std@@QAEXPAUElem_0040cfb0@@IABU3@@Z
InsertFn_0040cca0 g_insert_0040cca0 = &Vec_0040c5d0::insert;

typedef Vec_0040c5d0::iterator (Vec_0040c5d0::*EraseFn_0040cfb0)(
    Vec_0040c5d0::iterator, Vec_0040c5d0::iterator);
// FUNCTION: 0x40cfb0 ?erase@?$vector@UElem_0040cfb0@@V?$allocator@UElem_0040cfb0@@@std@@@std@@QAEPAUElem_0040cfb0@@PAU3@0@Z
EraseFn_0040cfb0 g_erase_0040cfb0 = &Vec_0040c5d0::erase;

typedef Vec_0040d000::size_type (Vec_0040d000::*SizeFn_0040d000)() const;
// FUNCTION: 0x40d000 ?size@?$vector@FV?$allocator@F@std@@@std@@QBEIXZ
SizeFn_0040d000 g_size_0040d000 = &Vec_0040d000::size;

typedef void (Vec_0040d000::*InsertFn_0040d020)(
    Vec_0040d000::iterator, Vec_0040d000::size_type, const short&);
// FUNCTION: 0x40d020 ?insert@?$vector@FV?$allocator@F@std@@@std@@QAEXPAFIABF@Z
InsertFn_0040d020 g_insert_0040d020 = &Vec_0040d000::insert;

typedef Vec_0040d000::iterator (Vec_0040d000::*EraseFn_0040d240)(
    Vec_0040d000::iterator, Vec_0040d000::iterator);
// FUNCTION: 0x40d240 ?erase@?$vector@FV?$allocator@F@std@@@std@@QAEPAFPAF0@Z
EraseFn_0040d240 g_erase_0040d240 = &Vec_0040d000::erase;

// FUNCTION: 0x40d290 ?insert@?$vector@EV?$allocator@E@std@@@std@@QAEXPAEIABE@Z
void (SimVector<unsigned char>::*g_insert_0040d290)(unsigned char*, unsigned int,
                                                      const unsigned char&) = &SimVector<unsigned char>::insert;

typedef Vec_0040d290::iterator (Vec_0040d290::*EraseFn_0040d470)(
    Vec_0040d290::iterator, Vec_0040d290::iterator);
// FUNCTION: 0x40d470 ?erase@?$vector@EV?$allocator@E@std@@@std@@QAEPAEPAE0@Z
EraseFn_0040d470 g_erase_0040d470 = &Vec_0040d290::erase;

typedef Vec_0040c7f0::size_type (Vec_0040c7f0::*SizeFn_0040d4c0)() const;
// FUNCTION: 0x40d4c0 ?size@?$vector@UElem_0040d550@@V?$allocator@UElem_0040d550@@@std@@@std@@QBEIXZ
SizeFn_0040d4c0 g_size_0040d4c0 = &Vec_0040c7f0::size;
