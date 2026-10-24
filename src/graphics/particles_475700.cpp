#include "win_types.h"
#include <stddef.h>
#include <stdlib.h>
#include <vector>

#include "timed_sub_particles.h"

struct Position_00475150 {
    short xFrac;
    short x;
    short yFrac;
    short y;
    short zFrac;
    short z;
};

#pragma pack(push, 1)
#include "../game/game.h"
#pragma pack(pop)

#include "../../link/globals.h"

int __stdcall GetGafFrameCount(void* ptr);
void* __stdcall GetGafFrame(void* a, int b);
void __stdcall DrawFrameBlended(void* dest, void* src, int x, int y);

struct TimedSubParticle {
    void* data;
    union {
        Vec3 pos;
        Position_00475150 posw;
    };
    int limit;
    int count;
    int period;
    int timer;

    void Step();
    void DrawParticle(int dest, short px, short py);
    int IsExpired(int unused);
    void Draw(void* dest, short px, short py)
    {
        short sy = posw.z - (posw.y >> 1) - py + 0x20;
        short sx = posw.x - px + 0x80;
        DrawFrameBlended(dest, GetGafFrame(data, count), sx, sy);
    }
};

class Vec_00476490 {
public:
    void insert(TimedSubParticle* pos, int count, const TimedSubParticle* src);
};

#include "particle_system.h"

// FUNCTION: 0x475700
void TimedSubParticles::Render(int dest)
{
    for (std::vector<TimedSubParticle>::iterator it = records.begin(); it != records.end(); ++it) {
        it->Draw((void*)dest, g_game->scrollX, g_game->scrollY);
    }
}
