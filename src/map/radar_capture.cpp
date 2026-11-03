#include "radar_capture.h"

#pragma pack(push, 1)

#include "../map/radar_types.h"

#include "../network/player_info.h"

#include "../network/player.h"

#include "../weapons/weapon_def.h"

#include "../weapons/unit_weapon_slot.h"

struct RadarUnitDefView {
    char unknown_0[0x204];
    short radardistance;
    short sonardistance;
    char unknown_208[0x20a - 0x208];
    short radardistancejam;
    short sonardistancejam;
    char unknown_20e[0x241 - 0x20e];
    InterceptorDefFlags flags1;
    unsigned char flags2;
};

#include "../units/unit.h"
#include "../weapons/projectile.h"

#include "../game/game.h"
#pragma pack(pop)

#include "../../link/globals.h"
#include "../../interfaces/services.h"
#include "../state/sim_radar.h"

static Player* Player_Get(unsigned char p)
{
    return &g_game->players_1b63_14b[0] + p;
}

static inline int ExploredOnRadar(Player* pi, int px, int py)
{
    int tx = px >> 5;
    int ty = py >> 5;
    if ((unsigned int)tx < (unsigned int)pi->exploredWidth && (unsigned int)ty < (unsigned int)pi->exploredHeight &&
        pi->explored[(unsigned int)pi->exploredWidth * ty + tx])
        return 1;
    return 0;
}

static inline int VisibleOnRadar(Player* pi, int px, int py)
{
    int tx = px >> 5;
    int ty = py >> 5;
    if (!((unsigned int)tx < (unsigned int)pi->exploredWidth && (unsigned int)ty < (unsigned int)pi->exploredHeight)) {
        return 0;
    }
    Player* b = pi;
    return (g_game->visibilityMask[(unsigned int)b->exploredWidth * ty + tx] &
            (1 << g_game->playerIndex)) != 0;
}

static inline int IsOnRadar(int px, int py)
{
    Player* pi = Player_Get(g_game->playerIndex);
    if ((g_game->mapFlags_14281.raw & 2) == 2)
        return ExploredOnRadar(pi, px, py);
    return VisibleOnRadar(pi, px, py);
}

static bool RadarUnitShown(const Unit* u, int enabled)
{
    return enabled != 0 || (u->flags & 0x300) != 0 || u->playerIndex == g_game->playerIndex;
}

static bool RadarProjectileShown(const Projectile* p, int px, int py, unsigned char* kind)
{
    if ((p->type->flags.value & 0x60000000) == 0) {
        if ((p->type->flags.value & 0x40) != 0)
            return false;
        if (!IsOnRadar(px, py) && p->owner != g_game->playerIndex)
            return false;
        *kind = RADAR_PROJECTILE_PIXEL;
        return true;
    }
    if (!IsOnRadar(px, py) && p->attacker->playerIndex != g_game->playerIndex)
        return false;
    *kind = RADAR_PROJECTILE_NUKE;
    return true;
}

static void CaptureUnit(RadarUnitCapture* out, const Unit* u)
{
    RadarUnitDefView* type = (RadarUnitDefView*)u->def.get();
    out->slot = (unsigned short)(u - g_game->units);
    out->id = u->id;
    out->xWhole = u->xWhole;
    out->yScaled = (int)u->zWhole - ((int)u->yWhole >> 1);
    out->color = Player_Get(u->playerIndex)->info->color;
    unsigned char flags = 0;
    if (u->recentlyDamagedTimer != 0)
        flags |= RADAR_UNIT_DAMAGED;
    if ((u->activateFlags & 1) != 0 || (type->flags2 & 4) == 0)
        flags |= RADAR_UNIT_SHOWS_RANGES;
    if (type->flags1.bit29)
        flags |= RADAR_UNIT_HAS_INTERCEPTORS;
    out->flags = flags;
    out->radarDistance = type->radardistance;
    out->sonarDistance = type->sonardistance;
    out->radarJamDistance = type->radardistancejam;
    out->sonarJamDistance = type->sonardistancejam;
    if (type->flags1.bit29) {
        for (int w = 0; w < 3; w++) {
            const UnitWeaponSlot* slot = &u->weapons[w];
            const WeaponDef* weapon = slot->weapon.get();
            if (weapon == nullptr)
                continue;
            out->weapons[w].interceptor = (unsigned char)weapon->flags.b.interceptor;
            out->weapons[w].coverage = weapon->coverage;
            out->weapons[w].stocked = slot->stockpile != 0;
        }
    }
}

void __stdcall RecordRadarUnits(void)
{
    int enabled;
    if (g_game->mapFlags_14281.bit0 || g_game->mapFlags_14281.bit1)
        enabled = 0;
    else
        enabled = 1;
    if ((g_game->matchFlags & 2))
        enabled = 1;

    unsigned int unitCount = 0;
    for (Unit* u = g_game->units; u != nullptr && u <= g_game->unitsEnd; u = (Unit*)((char*)u + 0x118)) {
        if (u->unitDefIndex != 0 && RadarUnitShown(u, enabled))
            unitCount++;
    }
    unsigned int projectileCount = 0;
    for (int i = 0; i < g_game->projectileCount; i++) {
        const Projectile* p = &g_game->projectiles[i];
        unsigned char kind;
        int py = p->positionWords.zWhole - ((int)p->positionWords.yWhole >> 1);
        if (RadarProjectileShown(p, p->positionWords.xWhole, py, &kind))
            projectileCount++;
    }

    unsigned int offset;
    unsigned char* at = (unsigned char*)SimCaptureAlloc(
        sizeof(RadarDrawCapture) + unitCount * sizeof(RadarUnitCapture) + projectileCount * sizeof(RadarProjectileCapture), &offset);
    if (at == nullptr)
        return;
    RadarDrawCapture* head = (RadarDrawCapture*)at;
    head->unitCount = unitCount;
    head->projectileCount = projectileCount;
    RadarUnitCapture* unitOut = (RadarUnitCapture*)(head + 1);
    for (Unit* u = g_game->units; u != nullptr && u <= g_game->unitsEnd; u = (Unit*)((char*)u + 0x118)) {
        if (u->unitDefIndex != 0 && RadarUnitShown(u, enabled))
            CaptureUnit(unitOut++, u);
    }
    RadarProjectileCapture* projectileOut = (RadarProjectileCapture*)unitOut;
    for (int i = 0; i < g_game->projectileCount; i++) {
        const Projectile* p = &g_game->projectiles[i];
        unsigned char kind;
        int px = p->positionWords.xWhole;
        int py = p->positionWords.zWhole - ((int)p->positionWords.yWhole >> 1);
        if (!RadarProjectileShown(p, px, py, &kind))
            continue;
        projectileOut->x = px;
        projectileOut->y = py;
        projectileOut->kind = kind;
        projectileOut->color = kind == RADAR_PROJECTILE_NUKE ? Player_Get(p->owner)->info->color : 0;
        projectileOut++;
    }
    unsigned int args[1] = {offset};
    RecordSimEvent(SIM_EVENT_RADAR_UNITS, 0, args, 1);
}

void __stdcall RecordRadarMapped(void)
{
    if (!g_game->timer.word.flags.mapChanged)
        return;
    g_game->timer.word.flags.mapChanged = 0;
    Player* t = &g_game->players_1b63_14b[g_game->playerIndex];
    unsigned int mask = 1 << g_game->playerIndex;
    int halfWidth = g_game->width / 2;
    int halfHeight = g_game->height / 2;
    unsigned int cells = (unsigned int)(halfWidth * halfHeight);
    unsigned int offset;
    unsigned char* at = (unsigned char*)SimCaptureAlloc(sizeof(RadarMappedCapture) + cells, &offset);
    if (at == nullptr)
        return;
    RadarMappedCapture* head = (RadarMappedCapture*)at;
    head->width = g_game->width;
    head->height = g_game->height;
    head->cells = cells;
    unsigned char* out = (unsigned char*)(head + 1);
    for (unsigned int i = 0; i < cells; i++) {
        if (!(unsigned short)(g_game->visibilityMask[i] & mask))
            out[i] = RADAR_CELL_HIDDEN;
        else if (t->explored[i])
            out[i] = RADAR_CELL_EXPLORED;
        else
            out[i] = RADAR_CELL_SEEN_NOW;
    }
    unsigned int args[1] = {offset};
    RecordSimEvent(SIM_EVENT_RADAR_MAPPED, 0, args, 1);
}
