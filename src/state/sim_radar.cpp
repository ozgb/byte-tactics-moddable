#include "sim_radar.h"
#include "sim_globals.h"
#include "block.h"
#include "../../interfaces/services.h"

static SimRadarArena* g_simRadar;

void SimCaptureCreate()
{
    g_simRadar = static_cast<SimRadarArena*>(SimAllocTagged("SIM RADAR", sizeof(SimRadarArena)));
    SimBlockTrack(reinterpret_cast<void**>(&g_simRadar));
}

SimRadarArena& SimCapture()
{
    if (g_simRadar == nullptr)
        SimCreate();
    return *g_simRadar;
}

void SimCaptureClear()
{
    SimRadarArena& a = SimCapture();
    a.used = 0;
    a.dropped = 0;
}

void* SimCaptureAlloc(unsigned int bytes, unsigned int* offset)
{
    SimRadarArena* arena = g_simRadar;
    if (arena == nullptr) {
        if (HarnessTestMode())
            FatalError("The sim radar arena does not exist.");
        return nullptr;
    }
    unsigned int size = (bytes + 7u) & ~7u;
    if (arena->used + size > SIM_RADAR_ARENA_BYTES) {
        if (HarnessTestMode())
            FatalError("The sim radar arena is full.");
        arena->dropped++;
        return nullptr;
    }
    *offset = arena->used;
    arena->used += size;
    if (arena->used > arena->highWater)
        arena->highWater = arena->used;
    return arena->bytes + *offset;
}
