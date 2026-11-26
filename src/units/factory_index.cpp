#include <vector>
#include <new>
#include "../state/block.h"
#include "../state/block_vector.h"
#include "../../interfaces/ref.h"

#include "../util/vec3.h"

#pragma pack(push,1)
#include "../network/player.h"
#include "../units/unit.h"
inline int Unit::Ready() { return (flags & 0x10000000) && !(flags & 0x4000); }
inline unsigned char Unit::PlayerIndex() { return ((Player*)player)->index; }

#include "../units/unit_def.h"

#include "../game/game.h"
#pragma pack(pop)

#include "../../link/globals.h"
#include "factory_index.h"
#include "../../interfaces/state_writer.h"
#include <stdio.h>
#include "../state/sim_globals.h"

static FactoryIndex* EnsureFactoryIndex()
{
    if (!g_factoryIndex)
        g_factoryIndex = new (kSim) FactoryIndex;
    return g_factoryIndex;
}

const BlockVector<Ref<Unit> >* FactoryIndexOf(int player)
{
    if (!g_factoryIndex)
        return nullptr;
    return &g_factoryIndex->players[player].factories;
}

static void WriteUnitRecords(const StateWriter* w);

void __stdcall UnitsWriteState(const StateWriter* w)
{
    for (int i = 0; g_factoryIndex && i < 10; i++) {
        FactoryIndexEntry& entry = g_factoryIndex->players[i];
        if (!entry.live)
            continue;
        char name[32];
        sprintf(name, "unitsFactories[%d]", i);
        w->Begin(25, name);
        w->Int("player", 0, 4, i);
        w->Int("factories.size", 0, 4, entry.factories.Size());
        unsigned int k = 0;
        for (BlockVector<Ref<Unit> >::iterator it = entry.factories.Begin(); it != entry.factories.End(); ++it, ++k)
            w->Handle("factories[%u]", k, STATE_HANDLE_UNIT, (*it).offset);
    }
    WriteUnitRecords(w);
}

static void WriteUnitRecords(const StateWriter* w)
{
    Unit* units = g_game->units.get();
    char name[64];
    for (unsigned int i = 0; units != nullptr && i < g_game->poolCount; i++) {
        Unit* u = &units[i];
        if (u->unitDefIndex == 0)
            continue;
        sprintf(name, "unitsState.units[%u]", i);
        w->Begin(54, name);
        w->Handle("head", 0, STATE_HANDLE_HEAP, u->head.offset);
        w->Int("hoverBobPhase", 0, 2, u->hoverBobPhase);
        w->Int("healthPercent", 0, 1, u->healthPercent);
        w->Int("prevHealthPercent", 0, 1, u->prevHealthPercent);
        w->Int("unfinishedInitZero", 0, 4, (unsigned int)u->unfinishedInitZero);
        w->Bytes("unknown_10a", 0, (const unsigned char*)u + 0x10a, 4);
        w->Int("zBufferFlag", 0, 4, (unsigned int)u->zBufferFlag);
    }
}

void __stdcall CreateFactoryIndex(int player)
{
    FactoryIndexEntry& entry = EnsureFactoryIndex()->players[player];
    entry.factories.Clear();
    entry.lastTick = 0;
    entry.live = 1;
}

void __stdcall FreeFactoryIndex(int player)
{
    if (!g_factoryIndex)
        return;
    FactoryIndexEntry& entry = g_factoryIndex->players[player];
    entry.factories.~BlockVector();
    new (&entry.factories) BlockVector<Ref<Unit> >();
    entry.live = 0;
}

void __stdcall RefreshFactoryIndex(int player)
{
    if (!g_factoryIndex)
        return;
    FactoryIndexEntry& entry = g_factoryIndex->players[player];
    if (!entry.live || (unsigned int)g_game->ticks < entry.lastTick + 0x1e)
        return;
    entry.factories.Clear();
    Player* owner = &g_game->players[player];
    for (Unit* u = g_game->units + 1; u != nullptr && u <= g_game->unitsEnd; ++u) {
        if (u->Ready() && owner->allied[u->PlayerIndex()] && u->PlayerIndex() == owner->index && u->buildLeft == 0.0 &&
            (u->def.get()->flags1 & 0x40) && (u->def.get()->flags1 & 0x200) && (u->activateFlags & 1))
            entry.factories.PushBack(u);
    }
    entry.lastTick = (unsigned int)g_game->ticks;
}

// FUNCTION: 0x40b530
int __stdcall GetFactoriesInRadius(int player, const Vec3* pos, int radius, Ref<Unit>* out, int capacity)
{
    BlockVector<Ref<Unit> >& list = g_factoryIndex->players[player].factories;
    radius *= radius;
    int count = 0;
    for (BlockVector<Ref<Unit> >::iterator it = list.Begin(); it != list.End(); ++it) {
        Unit* unit = *it;
        if ((unit->def->flags1 & 0x40) && (unit->def->flags1 & 0x200) && (unit->activateFlags & 1)) {
            int dz = pos->z - unit->pos.z;
            int dx = pos->x - unit->pos.x;
            int d = (int)(((__int64)dx * dx) >> 32) + (int)(((__int64)dz * dz) >> 32);
            if (d <= radius) {
                if (count < capacity)
                    out[count] = unit;
                count++;
            }
        }
    }
    return count;
}
