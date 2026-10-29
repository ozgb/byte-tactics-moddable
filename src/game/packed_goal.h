#ifndef PACKED_GOAL_H
#define PACKED_GOAL_H

#include "goal_owner.h"
#include "../ai/ai_search_goal.h"
#include "../orders/order_fx.h"

#pragma pack(push, 1)
class PackedGoal : public PathGoal {
public:
    Vec3 pos;
    Vec3 vel;
    short heading;
    char field_26;

    PackedGoal(GoalOwnerUnit* p);
    virtual void TickTowardGoal();
    virtual void ExportGoalPose(Vec3*, Vec3*, short*);
};
#pragma pack(pop)

#endif
