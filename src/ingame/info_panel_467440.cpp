#include "../util/vec3.h"

#include "../../interfaces/ref.h"
#pragma pack(push, 1)

union UnitPos_00467440 {
    Vec3 vec;
    short half[6];
};

#include "../units/unit_def.h"

#include "../network/player_info.h"

struct PlayerInfo_00467440;

#include "../units/unit.h"

struct MapSize_00467440 {
    unsigned int width;
    unsigned int height;

    int Contains(unsigned int tx, unsigned int ty)
    {
        return tx < width && ty < height;
    }
};

struct ByteMap_00467440 {
    Ref<unsigned char> data;
    MapSize_00467440 size;

    unsigned char Get(int x, int y) { return data[size.width * y + x]; }
};

struct PlayerInfo_00467440 {
    void* active;
    char unknown_4[0x27 - 0x4];
    Ref<PlayerInfo> data;
    char unknown_2b[0x67 - 0x2b];
    Ref<Unit> unitsBegin;
    Ref<Unit> unitsEnd;
    char unknown_6f[0x73 - 0x6f];
    char type;
    char unknown_74[0x7c - 0x74];
    ByteMap_00467440 explored;
    char unknown_88[0x108 - 0x88];
    unsigned char allied[1];
    char unknown_109[0x146 - 0x109];
    unsigned char index;
    char unknown_147[0x14b - 0x147];
};

#include "../game/game.h"
#pragma pack(pop)

class DetectionVisitor {
public:
    virtual void MarkUnitsInRadarOrSonarRadius(Unit* unit);
    int radarRangeSq;
    int sonarRangeSq;
    Vec3 pos;
};

class RadarJamVisitor {
public:
    virtual void ApplyRadarJamFlag(Unit* unit);
};

class SonarJamVisitor {
public:
    virtual void ApplySonarJamFlag(Unit* unit);
};

#include "../../link/globals.h"

#include "../../interfaces/seams.h"

static inline int IsExplored_00467440(PlayerInfo_00467440* p, UnitPos_00467440* pos)
{
    int tx = pos->half[1] >> 5;
    int ty = (pos->half[5] - (pos->half[3] >> 1)) >> 5;
    if (p->explored.size.Contains(tx, ty) && p->explored.Get(tx, ty) != 0)
        return 1;
    return 0;
}

static inline int IsSeen_00467440(PlayerInfo_00467440* p, UnitPos_00467440* pos)
{
    int tx = pos->half[1] >> 5;
    int ty = (pos->half[5] - (pos->half[3] >> 1)) >> 5;
    if (!p->explored.size.Contains(tx, ty))
        return 0;
    return (g_game->visibilityMask[p->explored.size.width * ty + tx] &
            (1 << g_game->playerIndex)) != 0;
}

// FUNCTION: 0x467440
void UpdateSensorRadarAndCloak(void)
{
    if (g_game->numPlayers < 2) {
        return;
    }
    unsigned char player = g_game->playerIndex;
    Unit* first = g_game->units + 1;
    Unit* last = g_game->unitsEnd;
    PlayerInfo_00467440* pl = &(*(PlayerInfo_00467440 (*)[10])&g_game->players)[g_game->playerIndex];
    Unit* u;

    Unit* a;
    for (a = first; a <= last; a++) {
        if (a->flags & 0x10000000) {
            a->flags &= ~0x1000;
            if (a->playerIndex == player
                || (((PlayerInfo_00467440*)a->player.get())->allied[pl->index] != 0
                    && (((PlayerInfo_00467440*)a->player.get())->data->flags_97 & 0x40) != 0)
                || (*(int*)pl != 0 && (pl->data->flags_9b & 0x40) != 0)) {
                a->flags |= 0x300;
            } else {
                a->flags &= ~0x700;
            }
        }
    }

    for (u = pl->unitsBegin; u <= pl->unitsEnd; u++) {
        if ((u->flags & 0x10000000) && !(u->flags & 0x4000) && (u->activateFlags & 1)) {
            if (u->def->radardistance != 0 || u->def->sonardistance != 0) {
                short a = u->def->radardistance;
                int t = a + u->yWhole * 2;
                short b = u->def->sonardistance;
                t = t * t;
                int s = (int)b * (int)b;
                if (a <= b) {
                    a = b;
                }
                Vec3* pp = &u->pos;
                DetectionVisitor v;
                v.radarRangeSq = t;
                v.sonarRangeSq = s;
                v.pos = u->pos;
                g_seams.map->VisitObjectsInRange((Vec3*)pp, (int)a << 16, (UnitVisitor*)&v);
            }
        }
    }

    for (u = first; u <= last; u++) {
        unsigned char ff;
        if ((u->flags & 0x10000000) && (ff = u->playerIndex, 1) && ff != pl->index && (u->activateFlags & 1)) {
            if (u->def->radardistancejam != 0) {
                int r = (int)u->def->radardistancejam << 16;
                Vec3* pp = &u->pos;
                RadarJamVisitor v;
                g_seams.map->VisitObjectsInRange((Vec3*)pp, r, (UnitVisitor*)&v);
            }
            if (u->def->sonardistancejam != 0) {
                int r2 = (int)u->def->sonardistancejam << 16;
                Vec3* pp2 = &u->pos;
                SonarJamVisitor v;
                g_seams.map->VisitObjectsInRange((Vec3*)pp2, r2, (UnitVisitor*)&v);
            }
        }
    }

    for (u = first; u <= last; u++) {
        if ((u->flags & 0x10000000) && ((PlayerInfo_00467440*)u->player.get())->active != 0) {
            char c = ((PlayerInfo_00467440*)u->player.get())->type;
            if (c == 1 || c == 2) {
                if (u->def->flags2 & 0x2000) {
                    if (g_seams.ai->HasReadyUnitInRange(u->playerIndex, (Vec3*)&u->pos, u->def->mincloakdistance)) {
                        int& b0 = u->workTime;
                        b0 = g_game->ticks + 0x5a;
                        u->flags |= 0x1000;
                    }
                }
            }
        }
    }

    for (u = first; u <= last; u++) {
        unsigned int f = u->flags;
        if ((f & 0x10000000) && !(f & 0x100) && !(u->activateFlags & 4)) {
            int pi = g_game->playerIndex;
            PlayerInfo_00467440* p2 = (PlayerInfo_00467440*)g_game->Current();
            unsigned int vis;
            if ((g_game->mapFlags & 2) == 2) {
                vis = IsExplored_00467440(p2, (UnitPos_00467440*)&u->pos);
            } else {
                vis = IsSeen_00467440(p2, (UnitPos_00467440*)&u->pos);
            }
            if ((int)vis) {
                u->flags = f | 0x100;
            }
        }
    }
}
