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

#include "../game/players.h"
#include "../../link/globals.h"
#include "line_of_sight_index.h"
#include "../../interfaces/state_writer.h"
#include <stdio.h>

#ifndef BT_REPLAY_INJECT_REFRESH_TICKS
#define BT_REPLAY_INJECT_REFRESH_TICKS 0x1e
#endif

static LosUnitIndex* s_index;

static LosUnitIndex* EnsureUnitIndex()
{
    if (s_index == nullptr) {
        s_index = new (kSim) LosUnitIndex;
        SimBlockTrack(reinterpret_cast<void**>(&s_index));
    }
    return s_index;
}

const LosUnitIndexEntry* LosUnitIndexOf(int player)
{
    if (s_index == nullptr)
        return nullptr;
    return &s_index->players[player];
}

static void WriteUnits(const StateWriter* w, const char* sizeName, const char* itemName, BlockVector<Ref<Unit> >& list)
{
    unsigned int n = list.Size();
    w->Int(sizeName, 0, 4, n);
    unsigned int k = 0;
    for (BlockVector<Ref<Unit> >::iterator it = list.Begin(); it != list.End(); ++it, ++k)
        w->Handle(itemName, k, STATE_HANDLE_UNIT, (*it).offset);
}

void WriteLosUnitIndex(const StateWriter* w)
{
    if (s_index == nullptr)
        return;
    for (int i = 0; i < 10; i++) {
        LosUnitIndexEntry& entry = s_index->players[i];
        if (!entry.live)
            continue;
        char name[32];
        sprintf(name, "losRadarIndex[%d]", i);
        w->Begin(24, name);
        w->Int("player", 0, 4, i);
        w->Int("hasSpecial", 0, 4, entry.hasSpecial);
        WriteUnits(w, "visible.size", "visible[%u]", entry.visible);
        WriteUnits(w, "known.size", "known[%u]", entry.known);
    }
}

static LosUnitIndexEntry* EntryOf(int player)
{
    return &s_index->players[player];
}

static void ReleaseUnits(BlockVector<Ref<Unit> >& list)
{
    list.~BlockVector();
    new (&list) BlockVector<Ref<Unit> >();
}

void __stdcall CreateUnitIndex(int player)
{
    LosUnitIndexEntry& entry = EnsureUnitIndex()->players[player];
    entry.visible.Clear();
    entry.known.Clear();
    entry.hasSpecial = 0;
    entry.lastTick = 0;
    entry.live = 1;
}

void __stdcall FreeUnitIndex(int player)
{
    if (s_index == nullptr)
        return;
    LosUnitIndexEntry& entry = s_index->players[player];
    ReleaseUnits(entry.visible);
    ReleaseUnits(entry.known);
    entry.live = 0;
}

void __stdcall RefreshUnitIndex(int player)
{
    if (s_index == nullptr)
        return;
    LosUnitIndexEntry& entry = s_index->players[player];
    if (!entry.live || (unsigned int)g_game->ticks < entry.lastTick + BT_REPLAY_INJECT_REFRESH_TICKS)
        return;
    entry.visible.Clear();
    entry.known.Clear();
    entry.hasSpecial = 0;
    Player* owner = &g_game->players[player];
    for (Unit* u = g_game->units + 1; u != nullptr && u <= g_game->unitsEnd; ++u) {
        if (u->Ready()) {
            if (!owner->allied[u->PlayerIndex()]) {
                if (IsUnitVisibleToPlayer(owner, u) && !(u->flags & 0x8000))
                    entry.visible.PushBack(u);
                if ((unsigned char)(u->flags >> 8) & 1)
                    entry.known.PushBack(u);
            } else if (u->PlayerIndex() == owner->index && u->buildLeft == 0.0) {
                if ((u->def.get()->flags1 & 0x400) && (u->activateFlags & 1))
                    entry.hasSpecial = 1;
            }
        }
    }
    entry.lastTick = (unsigned int)g_game->ticks;
}

// FUNCTION: 0x40ad80
int __stdcall GetVisibleEnemiesInRadius(int player, const Vec3* pos, int radius, int flags, Ref<Unit>* out, int capacity)
{
    int radius2 = radius * radius;
    int count = 0;
    LosUnitIndexEntry* entry = EntryOf(player);
    BlockVector<Ref<Unit> >::iterator it;
    BlockVector<Ref<Unit> >& visible = entry->visible;
    for (it = visible.Begin(); it != visible.End(); ++it) {
        Unit* unit = *it;
        int dz = pos->z - unit->pos.z;
        int dx = pos->x - unit->pos.x;
        int d = (int)(((__int64)dx * dx) >> 32) + (int)(((__int64)dz * dz) >> 32);
        if (d <= radius2 && unit->Ready()) {
            if (count < capacity)
                out[count] = unit;
            count++;
        }
    }
    if (entry->hasSpecial && count == 0) {
        BlockVector<Ref<Unit> >& known = entry->known;
        for (it = known.Begin(); it != known.End(); ++it) {
            Unit* unit = *it;
            int dz = pos->z - unit->pos.z;
            int dx = pos->x - unit->pos.x;
            int d = (int)(((__int64)dx * dx) >> 32) + (int)(((__int64)dz * dz) >> 32);
            if (d <= radius2 && unit->Ready()) {
                if (count < capacity)
                    out[count] = unit;
                count++;
            }
        }
    }
    return count;
}

// FUNCTION: 0x40b0d0
int __stdcall HasReadyUnitInRange(int player, const Vec3* p, int range)
{
    int r2 = range * range;
    BlockVector<Ref<Unit> >& units = EntryOf(player)->visible;
    BlockVector<Ref<Unit> >::iterator it = units.Begin();
    if (it != units.End()) {
        do {
            Unit* u = *it;
            int dz = p->z - u->pos.z;
            int dx = p->x - u->pos.x;
            if ((int)(((__int64)dx * dx) >> 32) + (int)(((__int64)dz * dz) >> 32) <= r2
                && (u->flags & 0x10000000)
                && !(u->flags & 0x4000))
                return 1;
        } while (++it != units.End());
    }
    return 0;
}

int __stdcall GetSeenUnitsInRadius(int player, const Vec3* pos, int radius, Ref<Unit>* out, int capacity)
{
    int count = 0;
    int r2 = radius * radius;
    BlockVector<Ref<Unit> >& units = EntryOf(player)->visible;
    for (BlockVector<Ref<Unit> >::iterator it = units.Begin(); it != units.End(); it++) {
        Unit* u = *it;
        int dz = pos->z - u->pos.z;
        int dx = pos->x - u->pos.x;
        int d = (int)(((__int64)dx * dx) >> 32) + (int)(((__int64)dz * dz) >> 32);
        if (d <= r2) {
            if (count < capacity)
                out[count] = u;
            count++;
        }
    }
    return count;
}

// FUNCTION: 0x408090
int __stdcall IsPointVisible(Player* player, Vec3* point)
{
    Vec3Words* pos = (Vec3Words*)point;
    int tx = pos->xWhole >> 5;
    int ty = (pos->zWhole - (pos->yWhole >> 1)) >> 5;
    if (!((unsigned int)tx < (unsigned int)player->exploredWidth && (unsigned int)ty < (unsigned int)player->exploredHeight)) {
        return 0;
    }
    return (g_game->visibilityMask[(unsigned int)player->exploredWidth * ty + tx] & (1 << g_game->playerIndex)) != 0;
}
