#include <string.h>
#include <math.h>
#include "win_types.h"
#include "../state/block.h"
#include "../../interfaces/host.h"
#include "../../interfaces/ref.h"
#include "../util/vec3.h"
#include "../../interfaces/seams.h"
#include "../../interfaces/services.h"

struct Unit;
class HapiBank;

static inline Vec3 MakeVec3(int ax, int ay, int az)
{
    Vec3 r;
    r.x = ax;
    r.y = ay;
    r.z = az;
    return r;
}

#pragma pack(push, 1)
#include "../units/unit.h"
#include "../game/game.h"
#pragma pack(pop)

extern Game* g_game;


#include "../units/unit_def.h"

#pragma pack(push, 1)
struct UnitDefSeaFlags {
    char unknown_0[0x241];
    unsigned int unknown_241_0 : 22;
    unsigned int seaUnit : 1;
    unsigned int unknown_241_1 : 9;
};
#pragma pack(pop)

#include "../../interfaces/services.h"


struct GoalOwnerUnit;

#include "unit_motion.h"
#include "../util/angles.h"

class AiSearchGoal;

#include "air_maneuver_order.h"

#include "packed_goal.h"
#include "path_order.h"
#include "order_goals.h"

// FUNCTION: 0x4905e0
// FUNCTION: 0x490630 ??_GPackedGoal@@UAEPAXI@Z
PackedGoal::PackedGoal(GoalOwnerUnit* p)
    : PathGoal((Unit*)p)
{
    pos = p->pos;
    vel = MakeVec3(0, 0, 0);
    heading = p->angles.heading;
}

// FUNCTION: 0x490650
void PackedGoal::ExportGoalPose(Vec3* outPos, Vec3* outVel,
                                  short* outHeading)
{
    *outPos = pos;
    *outVel = vel;
    *outHeading = heading;
}

// FUNCTION: 0x490690
void PackedGoal::TickTowardGoal()
{
    if (!target)
        return;
    Vec3 old = pos;
    ((OrderFx*)target)->FillWorldPos(&pos);
    vel = pos - old;
    int dist = (int)_hypot(owner->pos.x - pos.x, owner->pos.z - pos.z);
    if (dist > 0xa00000) {
        if (((UnitDefSeaFlags*)owner->def.get())->seaUnit)
            pos.y = (g_game->seaLevel + owner->def->altitude) << 16;
        else
            pos.y = (owner->def->altitude + ((unsigned char*)owner->spatialBucket.get())[1]) << 16;
    }
    if (dist > 0x1400000 || (!((OrderFx*)target)->GetDesiredHeading((unsigned short*)&heading) && dist > 0x100000))
        heading = (short)GetHeadingBetween(&owner->pos, &pos);
    if (((OrderFx*)target)->ContainsUnit((Unit*)owner)) {
        ((OrderFx*)target)->AddFlags(0x20);
        if (!((OrderFx*)target)->KeepAfterComplete())
            SetPathOrder(0);
    }
}

// FUNCTION: 0x4907e0
// FUNCTION: 0x490840 ??_GLiteGoal@@UAEPAXI@Z
LiteGoal::LiteGoal(Unit* p)
    : PackedGoal((GoalOwnerUnit*)p)
{
    dirty = 1;
    mode = 0;
}

// FUNCTION: 0x490860
void LiteGoal::SetPathOrder(void* param)
{
    PathGoal::SetPathOrder(param);
    dirty = 1;
}

// FUNCTION: 0x490880
void LiteGoal::TickTowardGoal()
{
    if ((((GoalMotionFlags*)owner->motion.get())->flags & 3) != mode)
        dirty = 1;
    PackedGoal::TickTowardGoal();
}

// FUNCTION: 0x4908b0
int LiteGoal::HasNetUnitState()
{
    return netDirtyFlags & 1;
}

// FUNCTION: 0x4908c0
void LiteGoal::SerializeNetUnitState(BitWriter* stream)
{
    if (target == 0) {
        stream->WriteBits(0, 2);
    } else if (((OrderFx*)target)->GetType() == 2) {
        stream->WriteBits(1, 2);
        ((OrderFx*)target)->SerializeToBits(stream);
    } else if (((OrderFx*)target)->GetType() == 3) {
        stream->WriteBits(2, 2);
        ((OrderFx*)target)->SerializeToBits(stream);
    }
    stream->WriteBits(((GoalMotionFlags*)owner->motion.get())->mode, 2);
    state = ((GoalMotionFlags*)owner->motion.get())->mode << 1;
}

// FUNCTION: 0x490940
PackedPosGoal::PackedPosGoal(Unit* p)
    : PackedGoal((GoalOwnerUnit*)p)
{
}

// FUNCTION: 0x4909a0 ??_GPackedPosGoal@@UAEPAXI@Z
// FUNCTION: 0x4909e0
PackedPosGoal::~PackedPosGoal()
{
    delete (OrderFx*)target;
    target = 0;
}

// FUNCTION: 0x490a10
void PackedPosGoal::DeserializeNetUnitState(BitReader* reader)
{
    if (target) {
        delete (OrderFx*)target;
        target = 0;
    }
    int kind = reader->ReadBits(2);
    if (kind == 1)
        target = new (kSim) PathOrder((Unit*)owner.get(), reader);
    else if (kind == 2)
        target = new (kSim) AirManeuverOrder(owner, reader);
    int state = reader->ReadBits(2);
    owner->motion->SetFlightMode(owner, state);
}
