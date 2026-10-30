#include "win_types.h"
#include <vector>
#include "../state/block_vector.h"
#include <math.h>
#include <io.h>

#include "../util/vec3.h"

#pragma pack(push,1)
#include "../network/player.h"
#include "../units/unit.h"
inline int Unit::Ready() { return (flags & 0x10000000) && !(flags & 0x4000); }
inline unsigned char Unit::PlayerIndex() { return ((Player*)player)->index; }

#include "../units/unit_def.h"

#include "../map/mission.h"

#include "../map/feature.h"

#include "../map/cell.h"

#include "../game/game.h"

#include "player_ai.h"
#pragma pack(pop)

#include "../../link/globals.h"

#include "../util/math.h"
int __stdcall CanPlaceUnitFootprint(UnitDef* type, short a, Point16 cell, int b);
int __stdcall CanBuildAt(UnitDef* type, Point16 cell, int a, int b);
void __stdcall PopHeapFirst(KeyedCell* first, KeyedCell* last, KeyedCell* dest,
                            KeyedCell val, int*);
#include "../../interfaces/seams.h"

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

static inline void PopHeap(KeyedCell* f, KeyedCell* l)
{
    PopHeapFirst(f, l - 1, l - 1, KeyedCell(*(l - 1)), (int*)0);
}

static inline Vec3 Direction(short angle, int scale)
{
    Vec3 v;
    v.x = -ScaledSin(angle, scale);
    v.y = 0;
    v.z = -ScaledCos(angle, scale);
    return v;
}

// FUNCTION: 0x40aa40
void PlayerAI::RefreshUnitLists()
{
    visible.Clear();
    factories.Clear();
    known.Clear();
    builders=0; hasSpecial=0;
    struct { float x,y,z,total; } sum={0,0,0,0};
    std::fill(counts.begin(),counts.end(),(short)0);
    for(Unit* u=g_game->units+1;u<=g_game->unitsEnd;++u) {
        if(u->Ready()) {
            if(!owner->allied[u->PlayerIndex()]) {
                if(g_seams.los_radar->IsUnitVisibleToPlayer(owner,u) && !(u->flags&0x8000)) visible.units.push_back(u);
                if((unsigned char)(u->flags>>8)&1) known.units.push_back(u);
            } else if(u->PlayerIndex()==owner->index && u->buildLeft==0.0) {
                ++counts[u->unitDefIndex];
                if(u->def.get()->ids.offset) ++builders;
                if((u->def.get()->flags1&0x40) && (u->def.get()->flags1&0x200) && (u->activateFlags&1)) factories.list.units.push_back(u);
                if((u->def.get()->flags1&0x400) && (u->activateFlags&1)) hasSpecial=1;
                float weight=(char)weights[u->unitDefIndex];
                sum.total+=weight;
#ifdef __clang__
                sum.x-=(double)u->pos.x*weight*-0.0000152587890625f;
                sum.y-=(double)u->pos.y*weight*-0.0000152587890625f;
                sum.z-=(double)u->pos.z*weight*-0.0000152587890625f;
                {
                    volatile float roundedSums[4]={sum.x,sum.y,sum.z,sum.total};
                    sum.x=roundedSums[0];
                    sum.y=roundedSums[1];
                    sum.z=roundedSums[2];
                    sum.total=roundedSums[3];
                }
#else
                sum.x-=(float)u->pos.x*weight*-0.0000152587890625f;
                sum.y-=(float)u->pos.y*weight*-0.0000152587890625f;
                sum.z-=(float)u->pos.z*weight*-0.0000152587890625f;
#endif
            }
        }
    }
    if(sum.total!=0.0f) { sum.x/=sum.total; sum.y/=sum.total; sum.z/=sum.total; }
#ifdef __clang__
    {
        volatile float roundedCentre[3]={sum.x,sum.y,sum.z};
        sum.x=roundedCentre[0];
        sum.y=roundedCentre[1];
        sum.z=roundedCentre[2];
    }
#endif
    int ix=(int)(sum.x*65536.0);
    int iy=(int)(sum.y*65536.0);
    int iz=(int)(sum.z*65536.0);
    centre=AiVec3(ix,iy,iz);
}
