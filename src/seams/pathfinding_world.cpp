#include <stddef.h>

#include "../../interfaces/seams.h"
#include "../../interfaces/ref.h"
#include "../state/block.h"
#include "../network/player.h"
#include "../game/game.h"
#include "../units/movement_class.h"
#include "../units/unit.h"
#include "../util/vec3.h"
#include "../state/sim_globals.h"
#include "pathfinding_world.h"

#include "../../link/globals.h"

class PathGoal;

#pragma pack(push, 1)
struct UnitMotionPlannerView {
    Ref<PathGoal> planner;
    Ref<MovementClass> owner;
};
#pragma pack(pop)

static Unit* UnitOf(PathUnit unit)
{
    Ref<Unit> ref;
    ref.offset = unit;
    return ref.get();
}

static MovementClass* ClassOf(PathClass moveClass)
{
    Ref<MovementClass> ref;
    ref.offset = moveClass;
    return ref.get();
}

static unsigned int __stdcall WorldTick()
{
    return (unsigned int)g_game->ticks;
}

static int __stdcall WorldPlayerCount()
{
    return g_game->numPlayers;
}

static int __stdcall WorldIsPlaying(int i)
{
    if (i >= 0 && i < 10) {
        Player* p = &g_game->players[i];
        if (p->active != 0 && (p->type == 1 || p->type == 2 || p->type == 3) && p->index != 10)
            return 1;
    }
    return 0;
}

static int __stdcall WorldUnitsPerPlayer()
{
    return g_game->unitsPerPlayer;
}

static PathUnit __stdcall WorldFirstUnit(int player)
{
    return g_game->players[player].unitsBegin.offset;
}

static PathUnit __stdcall WorldLastUnit(int player)
{
    return g_game->players[player].unitsEnd.offset;
}

static PathUnit __stdcall WorldNextUnit(PathUnit unit)
{
    return Ref<Unit>(UnitOf(unit) + 1).offset;
}

static int __stdcall WorldUnitState(PathUnit unit, PathUnitState* out)
{
    Unit* u = UnitOf(unit);
    UnitMotionPlannerView* motion = (UnitMotionPlannerView*)u->motion.get();
    if (((short)u->unitDefIndex) == 0 || motion == 0 || motion->owner.get() == 0)
        return 0;
    MovementClass* moveClass = motion->owner.get();
    Player* owner = u->player.get();
    out->player = owner != 0 ? (int)(owner - g_game->players) : 0;
    out->cell.x = u->cell.x;
    out->cell.y = u->cell.y;
    out->heading = (unsigned short)u->heading;
    out->moveClass = motion->owner.offset;
    out->footprintX = moveClass->footprintX;
    out->footprintY = moveClass->footprintZ;
    return 1;
}

static PathClass __stdcall WorldMoveClass(int index)
{
    MovementClass* entry = &g_movementClasses.entries[index & 31];
    if (entry->name == 0)
        return 0;
    return Ref<MovementClass>(entry).offset;
}

static int __stdcall WorldInBounds(PathClass moveClass, int x, int y)
{
    return g_seams.units->MovementClassInBounds(ClassOf(moveClass), (unsigned int)x, (unsigned int)y);
}

static int __stdcall WorldPass(PathClass moveClass, int x, int y)
{
    return g_seams.units->MovementClassGet(ClassOf(moveClass), x, y);
}

static void __stdcall WorldSeenSize(int* width, int* height)
{
    *width = g_game->width >> 1;
    *height = g_game->height >> 1;
}

static int __stdcall WorldSeen(int player, int x, int y)
{
    return ((1 << player) & g_game->visibilityMask[y * (g_game->width >> 1) + x]) != 0;
}

static int __stdcall WorldLineOfSight(PathClass moveClass, int x0, int y0, int x1, int y1)
{
    int dx = x1 - x0;
    int dy = y1 - y0;
    int nx = dx < 0 ? -dx : dx;
    int ny = dy < 0 ? -dy : dy;
    int sx = dx < 0 ? -1 : 1;
    int sy = dy < 0 ? -1 : 1;
    int x = x0;
    int y = y0;
    int ix = 0;
    int iy = 0;
    for (;;) {
        if (!WorldInBounds(moveClass, x, y) || WorldPass(moveClass, x, y) < 1)
            return 0;
        if (ix >= nx && iy >= ny)
            return 1;
        int decision = (1 + 2 * ix) * ny - (1 + 2 * iy) * nx;
        if (decision == 0) {
            if (!WorldInBounds(moveClass, x + sx, y) || WorldPass(moveClass, x + sx, y) < 1)
                return 0;
            if (!WorldInBounds(moveClass, x, y + sy) || WorldPass(moveClass, x, y + sy) < 1)
                return 0;
            x += sx;
            y += sy;
            ix++;
            iy++;
        } else if (decision < 0) {
            x += sx;
            ix++;
        } else {
            y += sy;
            iy++;
        }
    }
}

static void __stdcall WorldBeginSearch(PathUnit unit)
{
    Unit* u = UnitOf(unit);
    MovementClass* moveClass = ((UnitMotionPlannerView*)u->motion.get())->owner.get();
    g_seams.units->RefreshMovedUnits(moveClass, u);
}

static void __stdcall WorldEndSearch(PathUnit unit)
{
    Unit* u = UnitOf(unit);
    MovementClass* moveClass = ((UnitMotionPlannerView*)u->motion.get())->owner.get();
    g_seams.units->RefreshUnitIfStale(moveClass, u);
}

static PathWorldView s_world = {
    WorldTick, WorldPlayerCount, WorldIsPlaying, WorldUnitsPerPlayer, WorldFirstUnit, WorldLastUnit,
    WorldNextUnit, WorldUnitState, WorldMoveClass, WorldInBounds, WorldPass, WorldSeenSize, WorldSeen,
    WorldLineOfSight, WorldBeginSearch, WorldEndSearch,
};

PathWorldView* NativePathWorld()
{
    return &s_world;
}
