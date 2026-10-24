#include "../../interfaces/ref.h"
#pragma pack(push, 1)
struct MapSize_00465ac0 {
    unsigned int width;
    unsigned int height;
    int Contains(unsigned int tx, unsigned int ty) { return tx < width && ty < height; }
};
struct ByteMap_00465ac0 {
    Ref<unsigned char> data;
    MapSize_00465ac0 size;
    unsigned char Get(int x, int y) { return data[size.width * y + x]; }
};
struct Map_00465ac0 {
    char unknown_0[0x7c];
    ByteMap_00465ac0 explored;
};
#include "game.h"
#include "../units/unit_def.h"
#include "../units/unit.h"
struct Position_00465ac0 {
    short xFrac;
    short x;
    short yFrac;
    short y;
    short zFrac;
    short z;
};
struct Pos_00465ac0 {
    int x, y, z;
};
#pragma pack(pop)

#include "../../link/globals.h"

#include "../../interfaces/seams.h"

static inline int IsExplored(Map_00465ac0* map, Position_00465ac0* pos)
{
    int tx = pos->x >> 5;
    int ty = (pos->z - (pos->y >> 1)) >> 5;
    if (map->explored.size.Contains(tx, ty)) {
        if (map->explored.Get(tx, ty) != 0)
            return 1;
    }
    return 0;
}
static inline int IsSeen(Map_00465ac0* map, Position_00465ac0* pos)
{
    int tx = pos->x >> 5;
    int ty = (pos->z - (pos->y >> 1)) >> 5;
    if (!map->explored.size.Contains(tx, ty))
        return 0;
    return (g_game->visibilityMask[map->explored.size.width * ty + tx] &
            (1 << g_game->playerIndex)) != 0;
}

static inline int IsExplored2(Map_00465ac0* map, Position_00465ac0* pos)
{
    int tx = pos->x >> 5;
    int ty = (pos->z - (pos->y >> 1)) >> 5;
    if (map->explored.size.Contains(tx, ty)) {
        if (map->explored.data[map->explored.size.width * ty + tx] != 0)
            return 1;
    }
    return 0;
}
static inline int IsVisible(Map_00465ac0* map, Position_00465ac0* pos)
{
    return (g_game->mapFlags & 2) == 2 ? IsExplored(map, pos) : g_seams.ai->IsPointVisible((Player*)map, (Vec3*)pos);
}
static inline int IsVisible2(Map_00465ac0* map, Position_00465ac0* pos)
{
    return (g_game->mapFlags & 2) == 2 ? IsExplored2(map, pos) : g_seams.ai->IsPointVisible((Player*)map, (Vec3*)pos);
}
static inline int IsExplored3(Map_00465ac0* map, Position_00465ac0* pos)
{
    int tx = pos->x >> 5;
    int ty = (pos->z - (pos->y >> 1)) >> 5;
    if (map->explored.size.Contains(tx, ty)) {
        unsigned char* d = map->explored.data + map->explored.size.width * ty;
        if (d[tx] != 0)
            return 1;
    }
    return 0;
}

static inline int IsVisible3(Map_00465ac0* map, Position_00465ac0* pos)
{
    if ((g_game->mapFlags & 2) == 2)
        return IsExplored3(map, pos);
    return IsSeen(map, pos);
}
// FUNCTION: 0x465ac0
int __stdcall IsUnitVisibleToPlayer(Map_00465ac0* map, Unit* u)
{
    if ((Map_00465ac0*)u->player.get() == map)
        return 1;
    if (u->activateFlags & 4)
        return 0;
    Pos_00465ac0 p;
    p.x = u->def->modelMinX + u->pos.x;
    p.y = u->def->modelMaxY + u->pos.y;
    p.z = u->def->modelMinZ + u->pos.z;
    if (!(u->flags & 0x200)) {
        if (p.y < (g_game->seaLevel << 16))
            return 0;
    }
    if (IsVisible(map, (Position_00465ac0*)&p))
        return 1;
    p.x += u->def->modelSizeX;
    if (IsVisible2(map, (Position_00465ac0*)&p))
        return 1;
    p.z += u->def->modelSizeZ;
    p.y -= u->def->modelSizeY;
    if (IsVisible2(map, (Position_00465ac0*)&p))
        return 1;
    p.x -= u->def->modelSizeX;
    if (IsVisible3(map, (Position_00465ac0*)&p))
        return 1;
    return 0;
}
