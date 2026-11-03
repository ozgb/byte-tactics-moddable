#ifndef STATE_SIM_RADAR_H
#define STATE_SIM_RADAR_H

enum : unsigned int { SIM_RADAR_ARENA_BYTES = 2u << 20 };

struct SimRadarArena {
    unsigned int used;
    unsigned int dropped;
    unsigned int highWater;
    unsigned int reserved;
    unsigned char bytes[SIM_RADAR_ARENA_BYTES];
};

enum RadarUnitCaptureFlags : unsigned char {
    RADAR_UNIT_DAMAGED = 1,
    RADAR_UNIT_SHOWS_RANGES = 2,
    RADAR_UNIT_HAS_INTERCEPTORS = 4,
};

struct RadarWeaponCapture {
    int coverage;
    unsigned char interceptor;
    unsigned char stocked;
    unsigned short reserved;
};

struct RadarUnitCapture {
    unsigned short slot;
    unsigned short id;
    short xWhole;
    short reserved;
    int yScaled;
    unsigned char color;
    unsigned char flags;
    short radarDistance;
    short sonarDistance;
    short radarJamDistance;
    short sonarJamDistance;
    short reserved2;
    RadarWeaponCapture weapons[3];
};

enum RadarProjectileKind : unsigned char {
    RADAR_PROJECTILE_PIXEL = 1,
    RADAR_PROJECTILE_NUKE = 2,
};

struct RadarProjectileCapture {
    int x;
    int y;
    unsigned char kind;
    unsigned char color;
    unsigned short reserved;
};

struct RadarDrawCapture {
    unsigned int unitCount;
    unsigned int projectileCount;
};

enum RadarMappedCell : unsigned char {
    RADAR_CELL_HIDDEN = 0,
    RADAR_CELL_EXPLORED = 1,
    RADAR_CELL_SEEN_NOW = 2,
};

struct RadarMappedCapture {
    int width;
    int height;
    unsigned int cells;
};

static_assert(sizeof(RadarUnitCapture) == 48, "RadarUnitCapture size");
static_assert(sizeof(RadarProjectileCapture) == 12, "RadarProjectileCapture size");

SimRadarArena& SimCapture();

void SimCaptureCreate();

void SimCaptureClear();

void* SimCaptureAlloc(unsigned int bytes, unsigned int* offset);

#endif
