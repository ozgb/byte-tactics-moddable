#include "win_types.h"
#include <stddef.h>
#include <stdlib.h>
#include <vector>
#include <io.h>

#include "../util/vec3.h"

struct Position_00475470 {
    short xFrac;
    short x;
    short yFrac;
    short y;
    short zFrac;
    short z;
};

#pragma pack(push, 1)
#include "../network/player.h"

#include "../game/game.h"
#pragma pack(pop)

#include "../../link/globals.h"

int __stdcall GetGafFrameCount(void* ptr);
void* __stdcall GetGafFrame(void* a, int b);
void __stdcall DrawFrameBlended(void* dest, void* src, int x, int y);

static inline int IsExplored(Player* map, Position_00475470* pos)
{
    int tx = pos->x >> 5;
    int ty = (pos->z - (pos->y >> 1)) >> 5;
    if ((unsigned int)tx < (unsigned int)map->exploredWidth && (unsigned int)ty < (unsigned int)map->exploredHeight &&
        map->explored[(unsigned int)map->exploredWidth * ty + tx])
        return 1;
    return 0;
}

static inline int IsSeen(Player* map, Position_00475470* pos)
{
    int tx = pos->x >> 5;
    int ty = (pos->z - (pos->y >> 1)) >> 5;
    if (!((unsigned int)tx < (unsigned int)map->exploredWidth && (unsigned int)ty < (unsigned int)map->exploredHeight)) {
        return 0;
    }
    Player* b = map;
    return (g_game->visibilityMask[(unsigned int)b->exploredWidth * ty + tx] &
            (1 << g_game->playerIndex)) != 0;
}

static inline int IsVisible(Player* map, Position_00475470* pos)
{
    if ((g_game->mapFlags & 2) == 2)
        return IsExplored(map, pos);
    return IsSeen(map, pos);
}

struct SmokeParticle {
    void* data;
    union {
        Vec3 pos;
        Position_00475470 posw;
    };
    int limit;
    int count;
    int period;
    int timer;

    void Step();
    void DrawParticle(int dest, short px, short py);
    int IsExpired(int unused);
    void DrawIfVisible(void* dest, short px, short py)
    {
        short sx = posw.x - px + 0x80;
        short sy = posw.z - (posw.y >> 1) - py + 0x20;
        if (IsVisible(&g_game->players[g_game->playerIndex], &posw))
            DrawFrameBlended(dest, GetGafFrame(data, count), sx, sy);
    }
};

typedef std::vector<SmokeParticle> Vec_00474cd0;

class Class_00476210 {
public:
    void insert(Vec_00474cd0::iterator p, unsigned int m,
                const SmokeParticle& x);
};

#include "particle_system.h"

class SmokeParticles : public ParticleSystem {
public:
    int time;
    Vec_00474cd0 records;
    int emitPeriod;
    int holdPeriod;
    int maxFrame;
    int altAnimation;
    Vec3 pos;

    SmokeParticles();
    virtual void Update();
    virtual void Render(int);
    virtual int IsFinished();
    virtual void Emit();
    virtual int IsEmitDue();
    virtual void Init(Vec3* pos, int limit, int a, int b, int c,
                              int alt);
};

// FUNCTION: 0x475470
void SmokeParticles::Render(int dest)
{
    for (std::vector<SmokeParticle>::iterator it = records.begin(); it != records.end();
         ++it) {
        it->DrawIfVisible((void*)dest, g_game->scrollX, g_game->scrollY);
    }
}
