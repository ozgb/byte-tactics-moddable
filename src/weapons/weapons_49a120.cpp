#include <math.h>
#include <string.h>
#include "weapons_49a120.h"

union Fixed_0049a120 {
    int raw;
    struct {
        unsigned short frac;
        short whole;
    } part;
};

struct FixedVec3 {
    Fixed_0049a120 x;
    Fixed_0049a120 y;
    Fixed_0049a120 z;
};

struct CellPos_0049a120 {
    short x;
    short z;
};

#pragma pack(push, 1)

#include "weapon_def.h"
#include "projectile.h"
#include "../util/vec3.h"
#include "../../interfaces/ref.h"
#include "../units/unit.h"
#include "../units/unit_def.h"
#include "../network/player.h"

struct Cell {
    unsigned short unit;
    unsigned short unit2;
    char unknown_4[4];
    unsigned short feature;
    union {
        unsigned short spot;
        struct {
            unsigned char offsetZ;
            unsigned char offsetX;
        } origin;
    };
    unsigned char flags;
};

struct Spot_0049a120 {
    char unknown_0[8];
    Vec3 pos;
    char unknown_14[0x30 - 0x14];
};

struct FeatureDef_0049a120 {
    char unknown_0[0x100];
};

struct ProjectileDetonatePacket {
    unsigned char type;
    Vec3 aim;
    unsigned char weaponIndex;
};

struct Game_0049a120 {
    char unknown_0[0x141f3];
    int projectileCount;
    Ref<Projectile> projectiles;
    char unknown_141fb[0x1420b - 0x141fb];
    Ref<Spot_0049a120> spots;
    char unknown_1420f[0x14233 - 0x1420f];
    int mapWidthTiles;
    int mapHeightTiles;
    char unknown_1423b[0x1426f - 0x1423b];
    FeatureDef_0049a120* features;
    char pad_14273[0x14287 - 0x14273];
    Ref<Cell> heightMap;
    char unknown_1428b[0x14357 - 0x1428b];
    Ref<Unit> units;
};
#pragma pack(pop)

extern Game_0049a120* g_game;

#define WIN32_LEAN_AND_MEAN
#include "win_types.h"
#include <vector>

struct Hits_0049a120 {
    int numUnits;
    int numFeatures;
    Unit* units[20];
    Cell* features[64];

    int AddUnit(Unit* unit)
    {
        for (int k = 0; k < numUnits; k++) {
            if (units[k] == unit)
                return 0;
        }
        if (numUnits < 20) {
            units[numUnits] = unit;
            numUnits++;
        }
        return 1;
    }
    int AddFeature(Cell* cell)
    {
        for (int k = 0; k < numFeatures; k++) {
            if (features[k] == cell)
                return 0;
        }
        if (numFeatures < 64) {
            features[numFeatures] = cell;
            numFeatures++;
        }
        return 1;
    }
};

#include "weapons.h"
void __stdcall DetonateProjectile(Projectile* weapon, Unit* unit);
int __stdcall VectorLength(Vec3* v);
int __stdcall BroadcastPacket(int id, void* data, int size);

static inline int Length(Vec3* v)
{
    double x = v->x;
    double y = v->y;
    double z = v->z;
    return (int)sqrt(x * x + y * y + z * z);
}

static inline Unit* UnitFromId(unsigned short id)
{
    if (id == 0)
        return 0;
    return &g_game->units[id];
}

static inline Vec3 Sub(const Vec3& a, const Vec3& b)
{
    Vec3 r;
    r.x = a.x - b.x;
    r.y = a.y - b.y;
    r.z = a.z - b.z;
    return r;
}

#include "../../interfaces/seams.h"

// FUNCTION: 0x49a120
void __stdcall ApplyAreaDamage(Projectile* weapon, Vec3* pos)
{
    Hits_0049a120 hits;
    memset(&hits, 0, 8);
    int radius = weapon->type->areaOfEffect >> 1;
    int r = radius / 16 + 1;
    int cx = ((FixedVec3*)pos)->x.part.whole;
    int x0 = cx / 16 - r;
    int x1 = cx / 16 + r;
    int cz = ((FixedVec3*)pos)->z.part.whole;
    int z0 = cz / 16 - r;
    int z1 = cz / 16 + r;
    if (x0 < 0)
        x0 = 0;
    if (x1 > g_game->mapWidthTiles)
        x1 = g_game->mapWidthTiles;
    if (z0 < 0)
        z0 = 0;
    if (z1 > g_game->mapHeightTiles)
        z1 = g_game->mapHeightTiles;
    int enemyDamage = 0;
    int friendlyDamage = 0;

    for (int z = z0; z < z1; z++) {
        Cell* cell = g_seams.los_radar->GetMapCell(x0, z);
        for (int x = x0; x < x1; x++, cell++) {
            if (!cell)
                continue;
            for (int i = 0; i <= 1; i++) {
                Unit* unit = i ? UnitFromId(cell->unit2) : UnitFromId(cell->unit);
                if (!unit || unit == weapon->attacker)
                    continue;
                if (!hits.AddUnit(unit))
                    continue;
                UnitDef* def = unit->def;
                Vec3 d;
                {
                    int p = pos->x;
                    if (p < unit->pos.x + def->modelMinX) {
                        int lo = unit->pos.x + def->modelMinX;
                        d.x = lo - p;
                    } else if (p > unit->pos.x + def->modelMaxX) {
                        int hi = unit->pos.x + def->modelMaxX;
                        d.x = p - hi;
                    } else {
                        d.x = 0;
                    }
                }
                {
                    int p = pos->y;
                    if (p < unit->pos.y + def->modelMinY) {
                        int lo = unit->pos.y + def->modelMinY;
                        d.y = lo - p;
                    } else if (p > unit->pos.y + def->modelMaxY) {
                        int hi = unit->pos.y + def->modelMaxY;
                        d.y = p - hi;
                    } else {
                        d.y = 0;
                    }
                }
                {
                    int p = pos->z;
                    if (p < unit->pos.z + def->modelMinZ) {
                        int lo = unit->pos.z + def->modelMinZ;
                        d.z = lo - p;
                    } else if (p > unit->pos.z + def->modelMaxZ) {
                        int hi = unit->pos.z + def->modelMaxZ;
                        d.z = p - hi;
                    } else {
                        d.z = 0;
                    }
                }
                Fixed_0049a120 dist;
                dist.raw = Length(&d);
                int distance = dist.part.whole;
                if (distance < radius) {
                    float edge = weapon->type->edgeDamage;
                    float scale;
                    if (distance) {
#ifdef __clang__
                        double t = (double)distance / radius - 1.0f;
#else
                        float t = (float)distance / radius - 1.0f;
#endif
                        t *= t;
#ifdef __clang__
                        t = (1.0f - (double)edge) * t + edge;
#else
                        t = (1.0f - edge) * t + edge;
#endif
                        scale = t;
                    } else {
                        scale = 1.0f;
                    }
                    int damage = ApplyWeaponDamage(weapon, unit, scale);
                    if (weapon->owner == unit->playerIndex)
                        friendlyDamage += damage;
                    else
                        enemyDamage += damage;
                }
            }
            if (weapon->type->flags.value & 0x4000)
                continue;
            Cell* origin = cell;
            int fx = x;
            int fz = z;
            if (cell->feature == 0xfffe) {
                fx -= cell->origin.offsetX;
                fz -= cell->origin.offsetZ;
                origin = g_seams.los_radar->GetMapCell(fx, fz);
            }
            unsigned short feature = origin->feature;
            if (feature >= 0xfffb)
                continue;
            int index = cell - g_game->heightMap;
            CellPos_0049a120 cp;
            cp.x = index % g_game->mapWidthTiles;
            cp.z = index / g_game->mapWidthTiles;
            CellPos_0049a120 at = cp;
            Spot_0049a120* spot = &g_game->spots[cell->spot];
            int distance;
            if ((cell->flags & 1) && spot) {
                Vec3 v = Sub(*pos, spot->pos);
                Fixed_0049a120 dist;
                dist.raw = VectorLength(&v);
                distance = dist.part.whole;
            } else {
                Vec3 centre = g_seams.features->GetFootprintCentre((Point16*)&at, (Feature*)&g_game->features[feature]);
                Vec3 p = *(Vec3*)&centre;
                Vec3 v = Sub(*pos, p);
                Fixed_0049a120 dist;
                dist.raw = (int)sqrt((double)v.x * v.x + (double)v.y * v.y + (double)v.z * v.z);
                distance = dist.part.whole;
            }
            if (distance >= radius)
                continue;
            if (!hits.AddFeature(origin))
                continue;
            g_seams.features->DamageFeature(origin, fx, fz, weapon->type);
        }
    }

    if (weapon->type->flags.b.interceptor) {
        Projectile* other = g_game->projectiles;
        for (int j = 0; j < g_game->projectileCount; j++, other++) {
            if ((other->flags & 2) || other == weapon)
                continue;
            Vec3* p = &other->position;
            int dx = weapon->position.x - p->x;
            int dy = weapon->position.y - p->y;
            int dz = weapon->position.z - p->z;
            int reach = weapon->type->areaOfEffect;
            if ((int)(((__int64)dx * dx) >> 32) + (int)(((__int64)dy * dy) >> 32) + (int)(((__int64)dz * dz) >> 32)
                < reach * reach) {
                DetonateProjectile(other, 0);
                ProjectileDetonatePacket packet;
                packet.type = 0xe;
                packet.aim = other->target;
                packet.weaponIndex = other->type->index;
                BroadcastPacket(weapon->attacker->player->id, &packet, sizeof(packet));
                packet.type = 0xe;
                packet.aim = weapon->target;
                packet.weaponIndex = weapon->type->index;
                BroadcastPacket(weapon->attacker->player->id, &packet, sizeof(packet));
            }
        }
    }

    if (weapon->attacker)
        g_seams.ai->MarkOwnerNetDirtyFromDamageSplit(weapon->attacker->player->ai, 0, weapon, enemyDamage, friendlyDamage);
}
