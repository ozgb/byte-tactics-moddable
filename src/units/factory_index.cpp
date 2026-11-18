#include "win_types.h"
#include <vector>
#include <new>
#include "../state/block.h"
#include "../state/block_vector.h"

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

static FactoryIndex* s_index;

static FactoryIndex* EnsureIndex()
{
    if (s_index == nullptr) {
        s_index = new (kSim) FactoryIndex;
        SimBlockTrack(reinterpret_cast<void**>(&s_index));
    }
    return s_index;
}

const BlockVector<Ref<Unit> >* FactoryIndexOf(int player)
{
    if (s_index == nullptr)
        return nullptr;
    return &s_index->players[player].factories;
}

void __stdcall CreateFactoryIndex(int player)
{
    FactoryIndexEntry& entry = EnsureIndex()->players[player];
    entry.factories.Clear();
    entry.lastTick = 0;
    entry.live = 1;
}

void __stdcall FreeFactoryIndex(int player)
{
    if (s_index == nullptr)
        return;
    FactoryIndexEntry& entry = s_index->players[player];
    entry.factories.~BlockVector();
    new (&entry.factories) BlockVector<Ref<Unit> >();
    entry.live = 0;
}

void __stdcall RefreshFactoryIndex(int player)
{
    if (s_index == nullptr)
        return;
    FactoryIndexEntry& entry = s_index->players[player];
    if (!entry.live || (unsigned int)g_game->ticks < entry.lastTick + 0x1e)
        return;
    entry.factories.Clear();
    Player* owner = &g_game->players[player];
    for (Unit* u = g_game->units + 1; u <= g_game->unitsEnd; ++u) {
        if (u->Ready()) {
            if (!owner->allied[u->PlayerIndex()]) {
            } else if (u->PlayerIndex() == owner->index && u->buildLeft == 0.0) {
                if ((u->def.get()->flags1 & 0x40) && (u->def.get()->flags1 & 0x200) && (u->activateFlags & 1))
                    entry.factories.PushBack(u);
            }
        }
    }
    entry.lastTick = (unsigned int)g_game->ticks;
}

// FUNCTION: 0x40b530
void __stdcall GetFactoriesInRadius(int player, const Vec3* pos, int radius, std::vector<Unit*>* out)
{
    BlockVector<Ref<Unit> >& list = s_index->players[player].factories;
    radius *= radius;
    for (BlockVector<Ref<Unit> >::iterator it = list.Begin(); it != list.End(); ++it) {
        Unit* unit = *it;
        if ((unit->def->flags1 & 0x40) && (unit->def->flags1 & 0x200) && (unit->activateFlags & 1)) {
            int dz = pos->z - unit->pos.z;
            int dx = pos->x - unit->pos.x;
            int d = (int)(((__int64)dx * dx) >> 32) + (int)(((__int64)dz * dz) >> 32);
            if (d <= radius)
                out->push_back(*it);
        }
    }
}
