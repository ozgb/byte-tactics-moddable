#include <string.h>
#include "../state/load_data.h"

#include "../../interfaces/view.h"
#include "view_block.h"
#include "view_fill.h"

enum {
    GAME_PLAYERS = 0x1b63,
    GAME_WEAPONS = 0x2cf3,
    GAME_LOCAL_PLAYER = 0x2a42,
    GAME_PLAYER_INDEX = 0x2a43,
    GAME_PROJECTILE_COUNT = 0x141f3,
    GAME_PROJECTILES = 0x141f7,
    GAME_SPOT_ENTRIES = 0x1420b,
    GAME_FEATURE_COUNT = 0x14253,
    GAME_GRAVITY = 0x14263,
    GAME_FEATURES = 0x1426f,
    GAME_MAP_WIDTH = 0x14233,
    GAME_MAP_HEIGHT = 0x14237,
    GAME_VISIBILITY = 0x14273,
    GAME_SEA_LEVEL = 0x1427f,
    GAME_MAP_FLAGS = 0x14281,
    GAME_CELLS = 0x14287,
    GAME_POOL_COUNT = 0x14351,
    GAME_UNITS = 0x14357,
    GAME_DEF_COUNT = 0x1438f,
    GAME_DEFS = 0x1439b,
    GAME_SPRITE_SOURCES = 0x147eb,
    GAME_LOGOS = 0x148d7,
    GAME_TEXTURE_GAF_COUNT = 0x148df,
    GAME_TEXTURE_GAFS = 0x148e3,
    GAME_DEBRIS_COUNT = 0x1491b,
    GAME_DEBRIS = 0x1491f,
    GAME_SHADOW_SOURCES = 0x1ab8f,
    GAME_PIECES = 0x1ab9f,
    GAME_WIND_X = 0x37ecc,
    GAME_WIND_Z = 0x37ed4,
    GAME_MAX_UNITS = 0x37ee6,
    GAME_COMMANDER_DEATH = 0x37ef6,
    GAME_MATCH_FLAGS = 0x37f30,
    GAME_MISSION = 0x391e9,
    GAME_END_FLAGS = 0x3923b
};

enum {
    PLAYER_STRIDE = 0x14b,
    PLAYER_INFO = 0x27,
    PLAYER_TYPE = 0x73,
    PLAYER_EXPLORED = 0x7c,
    PLAYER_EXPLORED_WIDTH = 0x80,
    PLAYER_EXPLORED_HEIGHT = 0x84,
    PLAYER_EXPLORED_SIZE = 0x88,
    PLAYER_ENERGY = 0x8c,
    PLAYER_ENERGY_INCOME = 0x90,
    PLAYER_ENERGY_USAGE = 0x94,
    PLAYER_METAL = 0x98,
    PLAYER_METAL_INCOME = 0x9c,
    PLAYER_METAL_USAGE = 0xa0,
    PLAYER_ENERGY_CAPACITY = 0xa4,
    PLAYER_METAL_CAPACITY = 0xa8,
    PLAYER_ENERGY_PRODUCED = 0xac,
    PLAYER_METAL_PRODUCED = 0xb4,
    PLAYER_ENERGY_WASTED = 0xcc,
    PLAYER_METAL_WASTED = 0xd4,
    PLAYER_SHARE_METAL = 0xe4,
    PLAYER_SHARE_ENERGY = 0xe8,
    PLAYER_DISPLAY_TIMER = 0xf8,
    PLAYER_KILLS = 0xfc,
    PLAYER_LOSSES = 0xfe,
    PLAYER_COMMANDER_KILLS = 0x104,
    PLAYER_COMMANDER_LOSSES = 0x106,
    PLAYER_ALLIED = 0x108,
    PLAYER_UNITS_CREATED = 0x140,
    PLAYER_UNIT_COUNT = 0x144,
    PLAYER_RANK = 0x148,
    PLAYER_ACTIVE = 0x0,
    INFO_SIDE = 0x95,
    INFO_COLOR = 0x96,
    INFO_FLAGS = 0x9b,
    INFO_WATCHING = 0x80
};

enum {
    UNIT_STRIDE = 0x118,
    UNIT_WEAPONS = 0x4,
    UNIT_WEAPON_STRIDE = 0x1c,
    UNIT_WEAPON_DEF = 0xc,
    UNIT_STOCKPILE = 0x1e,
    UNIT_LIST = 0x5c,
    UNIT_LIST2 = 0x60,
    UNIT_HEADING = 0x66,
    UNIT_PITCH = 0x68,
    UNIT_X = 0x6a,
    UNIT_Y = 0x6e,
    UNIT_Z = 0x72,
    UNIT_CARRIER = 0x86,
    UNIT_SCRIPT = 0x9a,
    UNIT_DEF_INDEX = 0xa6,
    UNIT_ID = 0xa8,
    UNIT_GROUP = 0xac,
    UNIT_KILLS = 0xb8,
    UNIT_DAMAGED = 0xfa,
    UNIT_HOLDOFF = 0xfb,
    UNIT_OWNER = 0xff,
    UNIT_BUILD_LEFT = 0x104,
    UNIT_HEALTH = 0x108,
    UNIT_ACTIVATE = 0x10e,
    UNIT_FLAGS = 0x110
};

enum {
    DEF_STRIDE = 0x249,
    DEF_MIN_X = 350,
    DEF_MIN_Z = 358,
    DEF_MAX_Y = 366,
    DEF_SIZE_X = 374,
    DEF_SIZE_Y = 378,
    DEF_SIZE_Z = 382,
    WEAPON_STRIDE = 0x115,
    WEAPON_SPLASH = 0x7c,
    WEAPON_EXPLOSION = 0x78,
    WEAPON_RELOAD = 0xe4,
    WEAPON_LIMIT = 256
};

enum {
    ORDER_KIND = 0x4,
    ORDER_TARGET = 0x16,
    ORDER_X = 0x22,
    ORDER_Y = 0x26,
    ORDER_Z = 0x2a,
    ORDER_ID = 0x36,
    ORDER_PROGRESS = 0x3e,
    ORDER_FLAGS = 0x42,
    ORDER_CREATED = 0x46,
    ORDER_NEXT = 0x4a,
    ORDER_CHAIN_LIMIT = 4096,
    ORDER_WEAPON_BUILD = 0x80000,
    ORDER_TARGET_SHOWN = 0x8
};

enum {
    SCRIPT_TABLE = 0x8,
    SCRIPT_PIECES = 0x14,
    SCRIPT_OBJECT_STATE = 0x540,
    TABLE_PIECE_COUNT = 0x8,
    PIECE_LIMIT = 512,
    PIECE_BASE = 0x22,
    PIECE_STRIDE_STATE = 0x36
};

enum {
    CELL_STRIDE = 13,
    CELL_HEIGHT = 4,
    CELL_FEATURE = 8,
    CELL_SPOT = 10,
    CELL_FLAGS = 12,
    CELL_LIMIT = 1 << 22,
    CELL_NO_FEATURE = 0xfffb,
    FEATURE_STRIDE = 0x100,
    FEATURE_FOOTPRINT_X = 0x94,
    FEATURE_FOOTPRINT_Z = 0x96,
    FEATURE_FLAGS = 0xfe,
    FEATURE_FLAGS_HIGH = 0xff,
    SPOT_STRIDE = 0x30,
    SPOT_CAPACITY = 0x800,
    SPOT_ANIM_FRAME = 0x4,
    SPOT_SHADOW_FRAME = 0x10,
    SPOT_X = 0x8,
    SPOT_Y = 0xc,
    SPOT_Z = 0x10,
    SPOT_BANK = 0x20,
    SPOT_HEADING = 0x22,
    SPOT_PITCH = 0x24
};

enum {
    PROJECTILE_STRIDE = 0x6b,
    PROJECTILE_X = 0x4,
    PROJECTILE_FLAGS = 0x69,
    PROJECTILE_OWNER = 0x66,
    DEBRIS_STRIDE = 0x54,
    DEBRIS_LIMIT = 300,
    PIECE_RECORD_STRIDE = 0x34,
    PIECE_RECORD_LIMIT = 300,
    SPRITE_SOURCE_COUNT = 9,
    SHADOW_SOURCE_COUNT = 3,
    SPRITE_NONE = 0xffff
};

enum {
    MODEL_VERTEX_COUNT = 0x4,
    MODEL_FACE_COUNT = 0x8,
    MODEL_FIRST_FACE = 0xc,
    MODEL_VERTICES = 0x24,
    MODEL_FACES = 0x28,
    FACE_STRIDE = 0x20,
    FACE_COLOR = 0x0,
    FACE_COUNT = 0x4,
    FACE_INDICES = 0xc,
    FACE_PIC = 0x10,
    FACE_SRC = 0x18,
    FACE_FLAGS = 0x1c,
    FACE_FLAT = 1,
    FACE_SEQUENCE = 2,
    FACE_TEAM_COLOUR = 4,
    GAF_FILE_COUNT = 0x4,
    GAF_FILE_ENTRIES = 0xc,
    GAF_ENTRY_NAME = 0x8,
    GAF_ENTRY_FRAMES = 0x28,
    GAF_FRAME_STRIDE = 0x8,
    TEXTURE_CACHE = 256
};

enum {
    MISSION_TYPE = 0x0,
    UNIT_FLAG_ALIVE = 0x10000000,
    UNIT_FLAG_IMMOBILE = 0x20000000,
    UNIT_FLAG_CARGO_SELECTABLE = 0x40000000,
    UNIT_FLAG_SELECTABLE = 0x20,
    UNIT_FLAGS_SELECTION = 0x10,
    UNIT_FLAGS_VISIT = 0xc0,
    UNIT_FLAGS_MENU = 0x03c00000,
    UNIT_FLAGS_SONAR = 0x200,
    UNIT_FLAGS_SENSED = 0x300,
    ACTIVATE_CLOAKED = 4,
    END_FLAGS_KEPT = 0x64,
    EVENT_LIMIT = 4096
};

typedef char ViewCheckPlayerRows[VIEW_MAX_PLAYERS == 10 ? 1 : -1];

static const unsigned char* s_game;
static unsigned s_viewer;
static unsigned s_controller;
static int s_losOn;
static unsigned s_gridWidth;
static unsigned s_gridHeight;
static const unsigned char* s_mask;
static unsigned s_maskWords;
static const unsigned char* s_explored;
static unsigned s_exploredSize;
static int s_seaLevel;
static const unsigned char* s_units;
static unsigned s_poolCount;
static const unsigned char* s_defs;
static unsigned s_defCount;
static unsigned s_textureCacheKey[3];
static unsigned s_texturePic[TEXTURE_CACHE];
static const unsigned char* s_textureEntry[TEXTURE_CACHE];
static unsigned s_textureFrame[TEXTURE_CACHE];

static unsigned R32(const unsigned char* p, unsigned offset)
{
    unsigned v;
    memcpy(&v, p + offset, 4);
    return v;
}

static int RI32(const unsigned char* p, unsigned offset)
{
    int v;
    memcpy(&v, p + offset, 4);
    return v;
}

static unsigned short R16(const unsigned char* p, unsigned offset)
{
    unsigned short v;
    memcpy(&v, p + offset, 2);
    return v;
}

static short RS16(const unsigned char* p, unsigned offset)
{
    short v;
    memcpy(&v, p + offset, 2);
    return v;
}

static float RF32(const unsigned char* p, unsigned offset)
{
    float v;
    memcpy(&v, p + offset, 4);
    return v;
}

static const unsigned char* RRef(const unsigned char* p, unsigned offset)
{
    return ViewHostRef(R32(p, offset));
}

static int AddInt(int a, int b)
{
    return (int)((unsigned)a + (unsigned)b);
}

static int SubInt(int a, int b)
{
    return (int)((unsigned)a - (unsigned)b);
}

static const unsigned char* Plain(unsigned stored)
{
    return (const unsigned char*)(unsigned long)stored;
}

static const unsigned char* PlayerAt(unsigned slot)
{
    return s_game + GAME_PLAYERS + slot * PLAYER_STRIDE;
}

static const unsigned char* UnitAt(unsigned index)
{
    return s_units + index * UNIT_STRIDE;
}

static const unsigned char* UnitOf(const unsigned char* p)
{
    unsigned offset;
    if (p == 0 || s_units == 0 || p < s_units)
        return 0;
    offset = (unsigned)(p - s_units);
    if (offset % UNIT_STRIDE != 0 || offset / UNIT_STRIDE >= s_poolCount)
        return 0;
    return p;
}

static const unsigned char* DefAt(unsigned index)
{
    if (s_defs == 0 || index >= s_defCount)
        return 0;
    return s_defs + index * DEF_STRIDE;
}

static unsigned UnitIdPlusOne(const unsigned char* unit)
{
    if (unit == 0 || R16(unit, UNIT_DEF_INDEX) == 0)
        return 0;
    return (unsigned)R16(unit, UNIT_ID) + 1;
}

static int CellSight(int col, int row)
{
    unsigned index;
    if (col < 0 || row < 0 || (unsigned)col >= s_gridWidth || (unsigned)row >= s_gridHeight)
        return VIEW_SIGHT_HIDDEN;
    index = (unsigned)row * s_gridWidth + (unsigned)col;
    if (s_mask == 0 || index >= s_maskWords || ((R16(s_mask, index * 2) >> s_viewer) & 1) == 0)
        return VIEW_SIGHT_HIDDEN;
    if (s_explored != 0 && index < s_exploredSize && s_explored[index] != 0)
        return VIEW_SIGHT_IN_SIGHT;
    return VIEW_SIGHT_MAPPED;
}

static int InSight(int xWhole, int yWhole, int zWhole)
{
    int state = CellSight(xWhole >> 5, (zWhole - (yWhole >> 1)) >> 5);
    return s_losOn ? state == VIEW_SIGHT_IN_SIGHT : state != VIEW_SIGHT_HIDDEN;
}

static int InSightAt(int x, int y, int z)
{
    return InSight((short)(x >> 16), (short)(y >> 16), (short)(z >> 16));
}

static int UnitDrawn(const unsigned char* unit, const unsigned char* def)
{
    int x;
    int y;
    int z;
    int sizeX;
    int sizeY;
    int sizeZ;
    if ((unit[UNIT_ACTIVATE] & ACTIVATE_CLOAKED) != 0)
        return 0;
    x = AddInt(RI32(def, DEF_MIN_X), RI32(unit, UNIT_X));
    y = AddInt(RI32(def, DEF_MAX_Y), RI32(unit, UNIT_Y));
    z = AddInt(RI32(def, DEF_MIN_Z), RI32(unit, UNIT_Z));
    if ((R32(unit, UNIT_FLAGS) & UNIT_FLAGS_SONAR) == 0 && y < (s_seaLevel << 16))
        return 0;
    sizeX = RI32(def, DEF_SIZE_X);
    sizeY = RI32(def, DEF_SIZE_Y);
    sizeZ = RI32(def, DEF_SIZE_Z);
    if (InSightAt(x, y, z))
        return 1;
    x = AddInt(x, sizeX);
    if (InSightAt(x, y, z))
        return 1;
    z = AddInt(z, sizeZ);
    y = SubInt(y, sizeY);
    if (InSightAt(x, y, z))
        return 1;
    x = SubInt(x, sizeX);
    return InSightAt(x, y, z);
}

static void FillWorld(PlayerView* v)
{
    const unsigned char* mission = RRef(s_game, GAME_MISSION);
    unsigned flags;
    v->versionMajor = VIEW_VERSION_MAJOR;
    v->versionMinor = VIEW_VERSION_MINOR;
    v->tick = R32(s_game, 0x38a47);
    v->viewer = (unsigned char)s_viewer;
    v->controller = (unsigned char)s_controller;
    v->gameType = mission != 0 ? (unsigned char)R32(mission, MISSION_TYPE) : 0;
    v->commanderDeath = (unsigned char)R32(s_game, GAME_COMMANDER_DEATH);
    flags = R16(s_game, GAME_MAP_FLAGS);
    v->sightOptions = (unsigned short)(flags & 7);
    v->endFlags = (unsigned short)(R16(s_game, GAME_END_FLAGS) & END_FLAGS_KEPT);
    v->maxUnits = R16(s_game, GAME_MAX_UNITS);
    v->mapWidth = R32(s_game, GAME_MAP_WIDTH);
    v->mapHeight = R32(s_game, GAME_MAP_HEIGHT);
    v->seaLevel = s_game[GAME_SEA_LEVEL];
    v->windX = RI32(s_game, GAME_WIND_X);
    v->windZ = RI32(s_game, GAME_WIND_Z);
    v->gravity = RI32(s_game, GAME_GRAVITY);
}

static void FillPlayers(PlayerView* v)
{
    unsigned i;
    const unsigned char* viewer = s_viewer < VIEW_MAX_PLAYERS ? PlayerAt(s_viewer) : 0;
    for (i = 0; i < VIEW_MAX_PLAYERS; i++) {
        const unsigned char* p = PlayerAt(i);
        const unsigned char* info = RRef(p, PLAYER_INFO);
        ViewPlayer* out = &v->players[i];
        out->active = R32(p, PLAYER_ACTIVE) != 0;
        out->type = p[PLAYER_TYPE];
        if (info != 0) {
            out->side = info[INFO_SIDE];
            out->color = info[INFO_COLOR];
            out->watching = (info[INFO_FLAGS] & INFO_WATCHING) != 0;
        }
        out->allied = viewer != 0 && viewer[PLAYER_ALLIED + i] != 0;
        out->rank = p[PLAYER_RANK];
        out->kills = R16(p, PLAYER_KILLS);
        out->losses = R16(p, PLAYER_LOSSES);
        out->commanderKills = R16(p, PLAYER_COMMANDER_KILLS);
        out->commanderLosses = R16(p, PLAYER_COMMANDER_LOSSES);
        out->unitsCreated = R32(p, PLAYER_UNITS_CREATED);
        out->unitCount = R16(p, PLAYER_UNIT_COUNT);
        memcpy(&out->totalMetalProduced, p + PLAYER_METAL_PRODUCED, 8);
        memcpy(&out->totalEnergyProduced, p + PLAYER_ENERGY_PRODUCED, 8);
        memcpy(&out->metalWasted, p + PLAYER_METAL_WASTED, 8);
        memcpy(&out->energyWasted, p + PLAYER_ENERGY_WASTED, 8);
        if (i != s_viewer)
            continue;
        out->metal = RF32(p, PLAYER_METAL);
        out->energy = RF32(p, PLAYER_ENERGY);
        out->metalCapacity = RF32(p, PLAYER_METAL_CAPACITY);
        out->energyCapacity = RF32(p, PLAYER_ENERGY_CAPACITY);
        out->metalIncome = RF32(p, PLAYER_METAL_INCOME);
        out->metalUsage = RF32(p, PLAYER_METAL_USAGE);
        out->energyIncome = RF32(p, PLAYER_ENERGY_INCOME);
        out->energyUsage = RF32(p, PLAYER_ENERGY_USAGE);
        out->shareMetal = RF32(p, PLAYER_SHARE_METAL);
        out->shareEnergy = RF32(p, PLAYER_SHARE_ENERGY);
        out->displayTimer = R32(p, PLAYER_DISPLAY_TIMER);
    }
}

static void FillPieces(PlayerView* v, ViewUnit* out, const unsigned char* unit)
{
    const unsigned char* script = RRef(unit, UNIT_SCRIPT);
    unsigned count = 0;
    unsigned q;
    const unsigned char* state = 0;
    ViewPiece* pieces;
    if (script != 0) {
        const unsigned char* table = (const unsigned char*)(unsigned long)R32(script, SCRIPT_TABLE);
        state = RRef(script, SCRIPT_OBJECT_STATE);
        if (table != 0)
            count = R32(table, TABLE_PIECE_COUNT);
        if (count > PIECE_LIMIT || R32(script, SCRIPT_PIECES) == 0 || state == 0)
            count = 0;
    }
    if (ViewAddPieces(v, out, count) == 0)
        return;
    pieces = &v->pieces[out->firstPiece];
    for (q = 0; q < count; q++) {
        const unsigned char* t = state + PIECE_BASE + q * PIECE_STRIDE_STATE;
        pieces[q].x = RI32(t, 4);
        pieces[q].y = RI32(t, 8);
        pieces[q].z = RI32(t, 12);
        pieces[q].rotX = RS16(t, 0x10);
        pieces[q].rotY = RS16(t, 0x12);
        pieces[q].rotZ = RS16(t, 0x14);
        pieces[q].shown = (R16(t, 0x28) & 1) != 0;
    }
}

static unsigned ChainLength(const unsigned char* order)
{
    unsigned n = 0;
    while (order != 0 && n < ORDER_CHAIN_LIMIT) {
        n++;
        order = RRef(order, ORDER_NEXT);
    }
    return n;
}

static void FillOrders(PlayerView* v, ViewUnit* out, const unsigned char* first)
{
    unsigned count = ChainLength(first);
    unsigned n;
    ViewOrder* rows;
    const unsigned char* order = first;
    if (ViewAddOrders(v, out, count) == 0)
        return;
    rows = &v->orders[out->firstOrder];
    for (n = 0; n < count; n++) {
        rows[n].kind = order[ORDER_KIND];
        if ((ViewHostOrderDrawMask(order[ORDER_KIND]) & 1) != 0)
            rows[n].buildDefIndex = R16(order, ORDER_ID);
        rows[n].target = (unsigned short)UnitIdPlusOne(UnitOf(RRef(order, ORDER_TARGET)));
        rows[n].created = R32(order, ORDER_CREATED);
        rows[n].x = RI32(order, ORDER_X);
        rows[n].y = ViewHostOrderPosYUnset(order) != 0 ? 0 : RI32(order, ORDER_Y);
        rows[n].z = RI32(order, ORDER_Z);
        order = RRef(order, ORDER_NEXT);
    }
}

static unsigned WeaponBuildPercent(const unsigned char* unit)
{
    const unsigned char* order = RRef(unit, UNIT_LIST2);
    unsigned n;
    for (n = 0; order != 0 && n < ORDER_CHAIN_LIMIT; n++) {
        if ((R32(order, ORDER_FLAGS) & ORDER_WEAPON_BUILD) != 0) {
            unsigned slot = R32(order, ORDER_ID);
            const unsigned char* weapon;
            unsigned reload;
            int progress;
            int percent;
            if (slot > 2)
                return 0;
            weapon = RRef(unit, UNIT_WEAPONS + slot * UNIT_WEAPON_STRIDE + UNIT_WEAPON_DEF);
            if (weapon == 0)
                return 0;
            reload = R16(weapon, WEAPON_RELOAD);
            progress = RI32(order, ORDER_PROGRESS);
            if (reload == 0 || progress <= 0)
                return 0;
            percent = (int)((unsigned)progress * 100u / reload);
            return percent > 255 ? 255 : (unsigned)percent;
        }
        order = RRef(order, ORDER_NEXT);
    }
    return 0;
}

static void FillUnit(PlayerView* v, ViewUnit* out, const unsigned char* unit)
{
    unsigned owner = unit[UNIT_OWNER];
    unsigned flags = R32(unit, UNIT_FLAGS);
    int own = owner == s_viewer;
    const unsigned char* viewerRow = s_viewer < VIEW_MAX_PLAYERS ? PlayerAt(s_viewer) : 0;
    const unsigned char* carrier = UnitOf(RRef(unit, UNIT_CARRIER));
    const unsigned char* first = RRef(unit, UNIT_LIST);
    float build = RF32(unit, UNIT_BUILD_LEFT);
    unsigned relation = 0;
    out->id = R16(unit, UNIT_ID);
    out->unitDefIndex = R16(unit, UNIT_DEF_INDEX);
    out->owner = (unsigned char)owner;
    if (own)
        relation |= VIEW_RELATION_OWN;
    if (viewerRow != 0 && owner < VIEW_MAX_PLAYERS && viewerRow[PLAYER_ALLIED + owner] != 0)
        relation |= VIEW_RELATION_ALLIED;
    if (owner == s_controller)
        relation |= VIEW_RELATION_CONTROLLED;
    if ((flags & UNIT_FLAG_SELECTABLE) != 0 && build == 0.0f && R32(unit, UNIT_HOLDOFF) == 0
        && (carrier == 0 || (R32(carrier, UNIT_FLAGS) & UNIT_FLAG_CARGO_SELECTABLE) != 0))
        relation |= VIEW_RELATION_SELECTABLE;
    out->relation = (unsigned char)relation;
    out->ground = (flags & 3) == 1;
    flags &= ~(unsigned)(UNIT_FLAGS_SELECTION | UNIT_FLAGS_VISIT | UNIT_FLAGS_MENU);
    if (!own)
        flags &= UNIT_FLAGS_SONAR | UNIT_FLAG_ALIVE;
    out->flags = flags;
    out->activateFlags = unit[UNIT_ACTIVATE];
    out->recentlyDamaged = unit[UNIT_DAMAGED];
    out->immobile = (R32(unit, UNIT_FLAGS) & UNIT_FLAG_IMMOBILE) != 0;
    out->x = RI32(unit, UNIT_X);
    out->y = RI32(unit, UNIT_Y);
    out->z = RI32(unit, UNIT_Z);
    out->heading = RS16(unit, UNIT_HEADING);
    out->pitch = RS16(unit, UNIT_PITCH);
    out->health = RS16(unit, UNIT_HEALTH);
    out->buildLeft = build;
    out->carrier = (unsigned short)UnitIdPlusOne(carrier);
    if (own) {
        unsigned raw = R32(unit, UNIT_FLAGS);
        float account[4];
        out->group = (unsigned char)R32(unit, UNIT_GROUP);
        out->killCount = R16(unit, UNIT_KILLS);
        out->weaponBuild = (unsigned char)WeaponBuildPercent(unit);
        out->stockpile = unit[UNIT_STOCKPILE];
        if (first != 0) {
            out->orderKind = first[ORDER_KIND];
            if ((R32(first, ORDER_FLAGS) & ORDER_TARGET_SHOWN) != 0)
                out->orderTarget = (unsigned short)UnitIdPlusOne(UnitOf(RRef(first, ORDER_TARGET)));
        }
        out->moveOrder = (unsigned char)((raw >> 18) & 3);
        out->fireOrder = (unsigned char)((raw >> 20) & 3);
        out->cloakOrder = (unsigned char)((raw >> 11) & 1);
        ViewHostUnitAccount(unit, account);
        out->metalMake = account[0];
        out->metalUse = account[1];
        out->energyMake = account[2];
        out->energyUse = account[3];
    }
    FillPieces(v, out, unit);
    if (owner == s_controller)
        FillOrders(v, out, first);
}

static void FillBlip(PlayerView* v, const unsigned char* unit, int radarEnabled)
{
    unsigned flags = R32(unit, UNIT_FLAGS);
    unsigned owner = unit[UNIT_OWNER];
    ViewBlip* blip;
    unsigned bits = 0;
    if (!radarEnabled && (flags & UNIT_FLAGS_SENSED) == 0 && owner != s_viewer)
        return;
    blip = ViewAddBlip(v);
    if (blip == 0) {
        ViewHostFail("testmode: the view's blips table is full\n");
        return;
    }
    if ((flags & 0x100) != 0)
        bits |= VIEW_BLIP_RADAR;
    if ((flags & 0x200) != 0)
        bits |= VIEW_BLIP_SONAR;
    if (owner == s_viewer)
        bits |= VIEW_BLIP_OWN;
    if (unit[UNIT_DAMAGED] != 0)
        bits |= VIEW_BLIP_DAMAGED;
    blip->id = R16(unit, UNIT_ID);
    blip->owner = (unsigned char)owner;
    blip->flags = (unsigned char)bits;
    blip->x = RI32(unit, UNIT_X);
    blip->y = RI32(unit, UNIT_Y);
    blip->z = RI32(unit, UNIT_Z);
}

static void FillUnits(PlayerView* v)
{
    unsigned i;
    unsigned flags = R16(s_game, GAME_MAP_FLAGS);
    int radarEnabled = (flags & 3) == 0 || (s_game[GAME_MATCH_FLAGS] & 2) != 0;
    for (i = 0; s_units != 0 && i < s_poolCount; i++) {
        const unsigned char* unit = UnitAt(i);
        const unsigned char* def;
        ViewUnit* out;
        if (R16(unit, UNIT_DEF_INDEX) == 0)
            continue;
        FillBlip(v, unit, radarEnabled);
        def = DefAt(R16(unit, UNIT_DEF_INDEX));
        if (unit[UNIT_OWNER] != s_viewer && (def == 0 || !UnitDrawn(unit, def)))
            continue;
        out = ViewAddUnit(v);
        if (out == 0) {
            ViewHostFail("testmode: the view's units table is full\n");
            return;
        }
        FillUnit(v, out, unit);
    }
}

static int FootprintSighted(unsigned cellX, unsigned cellZ, int footprintX, int footprintZ, int height)
{
    short x = (short)(cellX << 4);
    short z = (short)(cellZ << 4);
    if (InSight(x, height, z))
        return 1;
    x = (short)(x + (footprintX << 4));
    z = (short)(z + (footprintZ << 4));
    return InSight(x, height, z);
}

static void FillFeatureSpot(ViewFeature* out, const unsigned char* spots, const unsigned char* def, unsigned spot)
{
    const unsigned char* s;
    if (spots == 0 || spot >= SPOT_CAPACITY)
        return;
    s = spots + spot * SPOT_STRIDE;
    if ((R16(def, FEATURE_FLAGS) & 1) != 0) {
        out->frame = R16(s, SPOT_ANIM_FRAME);
        out->shadowFrame = R16(s, SPOT_SHADOW_FRAME);
        return;
    }
    out->model = 1;
    out->x = RI32(s, SPOT_X);
    out->y = RI32(s, SPOT_Y);
    out->z = RI32(s, SPOT_Z);
    out->rotX = RS16(s, SPOT_PITCH);
    out->rotY = RS16(s, SPOT_HEADING);
    out->rotZ = RS16(s, SPOT_BANK);
}

static void FillFeatures(PlayerView* v)
{
    const unsigned char* cells = RRef(s_game, GAME_CELLS);
    const unsigned char* defs = RRef(s_game, GAME_FEATURES);
    const unsigned char* spots = RRef(s_game, GAME_SPOT_ENTRIES);
    unsigned featureCount = R32(s_game, GAME_FEATURE_COUNT);
    unsigned width = R32(s_game, GAME_MAP_WIDTH);
    unsigned height = R32(s_game, GAME_MAP_HEIGHT);
    unsigned n = width * height;
    unsigned i;
    unsigned x = 0;
    unsigned z = 0;
    if (n > CELL_LIMIT || cells == 0 || defs == 0 || width == 0)
        return;
    for (i = 0; i < n; i++) {
        const unsigned char* c = cells + i * CELL_STRIDE;
        unsigned feature = R16(c, CELL_FEATURE);
        if (feature < CELL_NO_FEATURE && feature < featureCount) {
            const unsigned char* def = defs + feature * FEATURE_STRIDE;
            unsigned cellFlags = c[CELL_FLAGS];
            ViewFeature* out = ViewAddFeature(v);
            if (out == 0)
                return;
            out->cellX = (unsigned short)x;
            out->cellZ = (unsigned short)z;
            out->featureIndex = (unsigned short)feature;
            out->drawn = 1;
            if ((def[FEATURE_FLAGS_HIGH] & 8) != 0 && ((cellFlags >> 3) & 0xf) != s_viewer
                && !FootprintSighted(x, z, RS16(def, FEATURE_FOOTPRINT_X), RS16(def, FEATURE_FOOTPRINT_Z),
                                     c[CELL_HEIGHT])) {
                out->drawn = 0;
            } else if ((cellFlags & 1) != 0) {
                FillFeatureSpot(out, spots, def, R16(c, CELL_SPOT));
            }
        }
        if (++x == width) {
            x = 0;
            z++;
        }
    }
}

static unsigned SpriteIndex(unsigned source)
{
    unsigned k;
    unsigned w;
    if (source == 0)
        return SPRITE_NONE;
    for (k = 0; k < SPRITE_SOURCE_COUNT; k++) {
        if (R32(s_game, GAME_SPRITE_SOURCES + k * 4) == source)
            return k;
    }
    for (w = 0; w < WEAPON_LIMIT; w++) {
        const unsigned char* weapon = s_game + GAME_WEAPONS + w * WEAPON_STRIDE;
        if (R32(weapon, WEAPON_SPLASH) == source)
            return SPRITE_SOURCE_COUNT + w * 2;
        if (R32(weapon, WEAPON_EXPLOSION) == source)
            return SPRITE_SOURCE_COUNT + w * 2 + 1;
    }
    return SPRITE_NONE;
}

static unsigned ShadowIndex(unsigned source)
{
    unsigned k;
    if (source == 0)
        return SPRITE_NONE;
    for (k = 0; k < SHADOW_SOURCE_COUNT; k++) {
        if (R32(s_game, GAME_SHADOW_SOURCES + k * 4) == source)
            return k;
    }
    return SPRITE_NONE;
}

static int FrameIn(const unsigned char* entry, unsigned pic, unsigned* frame)
{
    unsigned count = R16(entry, 0);
    unsigned f;
    for (f = 0; f < count; f++) {
        if (R32(entry, GAF_ENTRY_FRAMES + f * GAF_FRAME_STRIDE) == pic) {
            *frame = f;
            return 1;
        }
    }
    return 0;
}

static const unsigned char* FrameInGaf(const unsigned char* gaf, unsigned pic, unsigned* frame)
{
    int count;
    int e;
    if (gaf == 0)
        return 0;
    count = RS16(gaf, GAF_FILE_COUNT);
    for (e = 0; e < count; e++) {
        const unsigned char* entry = Plain(R32(gaf, GAF_FILE_ENTRIES + e * 4));
        if (entry != 0 && FrameIn(entry, pic, frame))
            return entry;
    }
    return 0;
}

static const unsigned char* FrameEntry(unsigned pic, unsigned* frame)
{
    const unsigned char* game = ViewHostGame();
    unsigned gafs = R32(game, GAME_TEXTURE_GAFS);
    unsigned count = R32(game, GAME_TEXTURE_GAF_COUNT);
    unsigned logos = R32(game, GAME_LOGOS);
    unsigned slot = (pic >> 3) % TEXTURE_CACHE;
    const unsigned char* entry = 0;
    int g;
    if (pic == 0)
        return 0;
    if (s_textureCacheKey[0] != gafs || s_textureCacheKey[1] != count || s_textureCacheKey[2] != logos) {
        memset(s_texturePic, 0, sizeof s_texturePic);
        s_textureCacheKey[0] = gafs;
        s_textureCacheKey[1] = count;
        s_textureCacheKey[2] = logos;
    }
    if (s_texturePic[slot] == pic) {
        *frame = s_textureFrame[slot];
        return s_textureEntry[slot];
    }
    for (g = 0; gafs != 0 && g < (int)count && entry == 0; g++)
        entry = FrameInGaf(Plain(R32(Plain(gafs), g * 4)), pic, frame);
    if (entry == 0)
        entry = FrameInGaf(Plain(logos), pic, frame);
    if (entry == 0)
        return 0;
    s_texturePic[slot] = pic;
    s_textureEntry[slot] = entry;
    s_textureFrame[slot] = *frame;
    return entry;
}

static int FillFace(PlayerView* v, int colour, unsigned flags, unsigned pic, unsigned src, unsigned count,
                    const unsigned char* indices, int teamColour)
{
    ViewDebrisFace* out = ViewGeometryFace(v);
    unsigned short* rows;
    const unsigned char* entry = 0;
    unsigned frame = 0;
    unsigned k;
    if (out == 0)
        return 0;
    rows = ViewGeometryIndices(v, out, indices != 0 ? count : 0);
    if (rows == 0)
        return 0;
    for (k = 0; k < out->indexCount; k++)
        rows[k] = R16(indices, k * 2);
    out->texture = VIEW_NO_TEXTURE;
    if ((flags & FACE_FLAT) != 0) {
        out->kind = VIEW_FACE_FILLED;
        out->colour = (unsigned)colour & 0xff;
        return 1;
    }
    if (count != 4)
        return 1;
    out->kind = VIEW_FACE_TEXTURED;
    if ((flags & FACE_SEQUENCE) != 0) {
        entry = Plain(src);
        frame = (flags & FACE_TEAM_COLOUR) != 0 && teamColour >= 0 ? (unsigned)teamColour : (pic & 0xffff);
    } else {
        entry = FrameEntry(pic, &frame);
    }
    if (entry == 0)
        return 1;
    out->texture = (unsigned short)ViewGeometryTexture(v, (const char*)entry + GAF_ENTRY_NAME);
    out->frame = (unsigned short)frame;
    return out->texture != VIEW_NO_TEXTURE;
}

static unsigned DebrisFacePic(const ViewReportFace* f)
{
    if ((f->flags & (FACE_FLAT | FACE_SEQUENCE)) != 0)
        return f->pic;
    return (unsigned)LoadNative(f->pic);
}

static unsigned DebrisFaceSrc(const ViewReportFace* f)
{
    if ((f->flags & FACE_FLAT) != 0 || (f->flags & FACE_SEQUENCE) == 0)
        return f->src;
    return (unsigned)LoadNative(f->src);
}

int ViewFillDebrisGeometry(PlayerView* v, ViewDebris* out, const ViewReportPiece* piece)
{
    ViewGeometryMark mark;
    ViewDebrisVertex* vertices;
    unsigned vertexCount = piece->vertexCount < 0 ? 0 : (unsigned)piece->vertexCount;
    unsigned faceCount = piece->faceCount < 0 ? 0 : (unsigned)piece->faceCount;
    unsigned q;
    if (!ViewGeometryBegin(v, &mark))
        return 0;
    if (vertexCount > VIEW_REPORT_PIECE_VERTICES)
        vertexCount = VIEW_REPORT_PIECE_VERTICES;
    if (faceCount > VIEW_REPORT_PIECE_FACES)
        faceCount = VIEW_REPORT_PIECE_FACES;
    vertices = ViewGeometryVertices(v, vertexCount);
    if (vertices == 0)
        goto fail;
    for (q = 0; q < vertexCount; q++) {
        vertices[q].x = piece->vertices[q * 3];
        vertices[q].y = piece->vertices[q * 3 + 1];
        vertices[q].z = piece->vertices[q * 3 + 2];
    }
    for (q = piece->firstFace != -1 ? 1 : 0; q < faceCount; q++) {
        const ViewReportFace* f = &piece->faces[q];
        if (!FillFace(v, f->color, f->flags, DebrisFacePic(f), DebrisFaceSrc(f), f->count < 0 ? 0 : (unsigned)f->count, Plain(f->indices), -1))
            goto fail;
    }
    ViewGeometryEnd(v, &mark, &out->firstVertex, &out->vertexCount, &out->firstFace, &out->faceCount);
    return 1;
fail:
    ViewGeometryFail(v, &mark, &out->firstVertex, &out->vertexCount, &out->firstFace, &out->faceCount);
    return 0;
}

int ViewFillExplodedGeometry(PlayerView* v, ViewExplodedPiece* out, const unsigned char* desc, int teamColour)
{
    ViewGeometryMark mark;
    ViewDebrisVertex* vertices;
    const unsigned char* from;
    const unsigned char* faces;
    int vertexCount;
    int faceCount;
    int q;
    if (!ViewGeometryBegin(v, &mark))
        return 0;
    if (desc == 0)
        return 1;
    vertexCount = RI32(desc, MODEL_VERTEX_COUNT);
    faceCount = RI32(desc, MODEL_FACE_COUNT);
    from = Plain(R32(desc, MODEL_VERTICES));
    faces = Plain(R32(desc, MODEL_FACES));
    if (vertexCount < 0 || vertexCount > VIEW_MAX_DEBRIS_VERTICES || faceCount < 0 || faceCount > VIEW_MAX_DEBRIS_FACES
        || (vertexCount != 0 && from == 0) || (faceCount != 0 && faces == 0))
        goto fail;
    vertices = ViewGeometryVertices(v, (unsigned)vertexCount);
    if (vertices == 0)
        goto fail;
    for (q = 0; q < vertexCount; q++) {
        vertices[q].x = RI32(from, q * 12);
        vertices[q].y = RI32(from, q * 12 + 4);
        vertices[q].z = RI32(from, q * 12 + 8);
    }
    for (q = RI32(desc, MODEL_FIRST_FACE) != -1 ? 1 : 0; q < faceCount; q++) {
        const unsigned char* f = faces + q * FACE_STRIDE;
        int count = RI32(f, FACE_COUNT);
        if (!FillFace(v, RI32(f, FACE_COLOR), R32(f, FACE_FLAGS), R32(f, FACE_PIC), R32(f, FACE_SRC),
                      count < 0 ? 0 : (unsigned)count, Plain(R32(f, FACE_INDICES)), teamColour))
            goto fail;
    }
    ViewGeometryEnd(v, &mark, &out->firstVertex, &out->vertexCount, &out->firstFace, &out->faceCount);
    return 1;
fail:
    ViewGeometryFail(v, &mark, &out->firstVertex, &out->vertexCount, &out->firstFace, &out->faceCount);
    return 0;
}

static void FillProjectiles(PlayerView* v, const ViewWeaponsReport* report)
{
    const unsigned char* base = RRef(s_game, GAME_PROJECTILES);
    unsigned count = R32(s_game, GAME_PROJECTILE_COUNT);
    unsigned i;
    if (base == 0)
        return;
    if (count > VIEW_REPORT_PROJECTILES) {
        ViewHostFail("testmode: the view's projectiles table is full\n");
        return;
    }
    for (i = 0; i < count; i++) {
        const unsigned char* p = base + i * PROJECTILE_STRIDE;
        const ViewReportProjectile* extra = &report->projectiles[i];
        const unsigned char* weapon;
        ViewProjectile* out;
        int x;
        int y;
        int z;
        if (!report->projectileUsed[i] || (R16(p, PROJECTILE_FLAGS) & 2) != 0 || extra->active != 0)
            continue;
        x = RI32(p, PROJECTILE_X);
        y = RI32(p, PROJECTILE_X + 4);
        z = RI32(p, PROJECTILE_X + 8);
        if (!InSightAt(x, y, z))
            continue;
        out = ViewAddProjectile(v);
        if (out == 0) {
            ViewHostFail("testmode: the view's projectiles table is full\n");
            return;
        }
        weapon = RRef(p, 0);
        if (weapon != 0 && weapon >= s_game + GAME_WEAPONS)
            out->weaponIndex = (unsigned short)((unsigned)(weapon - (s_game + GAME_WEAPONS)) / WEAPON_STRIDE);
        out->owner = p[PROJECTILE_OWNER];
        out->x = x;
        out->y = y;
        out->z = z;
        out->startX = extra->startX;
        out->startY = extra->startY;
        out->startZ = extra->startZ;
        out->rotX = extra->pitch;
        out->rotY = extra->heading;
        out->rotZ = extra->bank;
        out->propellerSpin = extra->propellerSpin;
        out->groundY = extra->radius;
        out->spawnTick = extra->spawnTick;
        out->time = extra->time;
    }
}

static void FillDebris(PlayerView* v, const ViewWeaponsReport* report)
{
    unsigned d;
    for (d = 0; d < VIEW_REPORT_DEBRIS; d++) {
        const ViewReportDebris* e = &report->debris[d];
        const unsigned char* obj;
        ViewDebris* out;
        if (!e->used)
            continue;
        out = ViewAddDebris(v);
        if (out == 0) {
            ViewHostFail("testmode: the view's debris table is full\n");
            return;
        }
        obj = ViewHostRef(e->obj);
        out->sprite = (unsigned short)SpriteIndex(e->spriteSource);
        out->frame = e->frame;
        out->shadowSprite = (unsigned short)ShadowIndex(e->shadowSource);
        out->shadowFrame = e->shadowFrame;
        if (obj != 0 && obj >= s_game + GAME_PIECES) {
            unsigned offset = (unsigned)(obj - (s_game + GAME_PIECES));
            if (offset % PIECE_RECORD_STRIDE == 0 && offset / PIECE_RECORD_STRIDE < PIECE_RECORD_LIMIT) {
                const ViewReportPiece* piece = &report->pieces[offset / PIECE_RECORD_STRIDE];
                if (piece->used)
                    ViewFillDebrisGeometry(v, out, piece);
            }
        }
        out->x = e->x;
        out->y = e->y;
        out->z = e->z;
        out->rotX = e->angleX;
        out->rotY = e->angleY;
        out->rotZ = e->angleZ;
    }
}

static void FillExplodedPieces(PlayerView* v, const ViewWeaponsReport* report)
{
    unsigned s;
    for (s = 0; s < VIEW_REPORT_EXPLODED; s++) {
        const ViewReportExploded* e = &report->exploded[s];
        const unsigned char* unit;
        ViewExplodedPiece* out;
        int teamColour = 0;
        if (!e->used)
            continue;
        out = ViewAddExplodedPiece(v);
        if (out == 0) {
            ViewHostFail("testmode: the view's exploded pieces table is full\n");
            return;
        }
        unit = ViewHostRef(e->unit);
        if (unit != 0) {
            out->owner = unit[UNIT_OWNER];
            if (out->owner < VIEW_MAX_PLAYERS) {
                const unsigned char* info = RRef(PlayerAt(out->owner), PLAYER_INFO);
                if (info != 0)
                    teamColour = info[INFO_COLOR];
            }
        }
        ViewFillExplodedGeometry(v, out, Plain(e->desc), teamColour);
        if ((e->flags & 2) != 0)
            out->trail = 1;
        else if ((e->flags & 1) != 0)
            out->trail = 2;
        out->x = e->x;
        out->y = e->y;
        out->z = e->z;
        out->rotX = e->angle10;
        out->rotY = e->angle12;
        out->rotZ = e->angle14;
    }
}

int ViewIsMessageKind(unsigned kind)
{
    switch (kind) {
    case 1:
    case 2:
    case 12:
    case 13:
    case 14:
    case 21:
    case 22:
    case 23:
    case 25:
    case 26:
    case 30:
    case 31:
    case 33:
        return 1;
    }
    return 0;
}

static int EventReaches(const ViewRawEvent* e)
{
    const unsigned char* unit;
    switch (e->kind) {
    case 30:
    case 31:
        return s_viewer < 32 && ((e->args[4] >> s_viewer) & 1) != 0;
    case 1:
        if (s_units == 0 || e->args[0] >= s_poolCount)
            return 0;
        unit = UnitAt(e->args[0]);
        return R16(unit, UNIT_DEF_INDEX) != 0 && (R32(unit, UNIT_FLAGS) & UNIT_FLAG_ALIVE) != 0
            && unit[UNIT_OWNER] == s_viewer;
    case 23:
        return e->args[0] == s_viewer;
    case 12:
        return e->args[1] == s_controller || e->args[2] == s_controller;
    case 25:
    case 26:
        return e->args[0] == s_controller;
    }
    return ViewIsMessageKind(e->kind);
}

static void FillMessages(PlayerView* v)
{
    const ViewRawEvent* events;
    unsigned count = ViewHostTakeEvents(&events);
    unsigned n;
    unsigned a;
    for (n = 0; n < count; n++) {
        const ViewRawEvent* e = &events[n];
        ViewMessage* out;
        if (!EventReaches(e))
            continue;
        out = ViewAddMessage(v);
        if (out == 0) {
            ViewHostFail("testmode: the view's messages table is full\n");
            return;
        }
        out->kind = e->kind;
        out->flags = e->flags;
        for (a = 0; a < 6; a++)
            out->args[a] = e->args[a];
        if (e->count > 6) {
            out = ViewAddMessage(v);
            if (out == 0) {
                ViewHostFail("testmode: the view's messages table is full\n");
                return;
            }
            out->kind = e->kind;
            out->flags = 0x8000;
            for (a = 0; a < 6; a++)
                out->args[a] = e->args[6 + a];
        }
    }
}

static void FillSight(PlayerView* v)
{
    unsigned cells = ViewSightCells(v);
    unsigned k;
    for (k = 0; k < cells; k++) {
        unsigned state = VIEW_SIGHT_HIDDEN;
        if (s_mask != 0 && k < s_maskWords && ((R16(s_mask, k * 2) >> s_viewer) & 1) != 0) {
            state = VIEW_SIGHT_MAPPED;
            if (s_explored != 0 && k < s_exploredSize && s_explored[k] != 0)
                state = VIEW_SIGHT_IN_SIGHT;
        }
        v->sight[k] = (unsigned char)state;
    }
}

void ViewFill(PlayerView* v)
{
    const ViewWeaponsReport* report = ViewHostWeaponsReport();
    const unsigned char* viewer;
    unsigned width;
    unsigned height;
    unsigned n;
    s_game = ViewHostGame();
    s_viewer = s_game[GAME_PLAYER_INDEX];
    s_controller = s_game[GAME_LOCAL_PLAYER];
    s_losOn = (R16(s_game, GAME_MAP_FLAGS) & 2) != 0;
    s_seaLevel = s_game[GAME_SEA_LEVEL];
    s_units = RRef(s_game, GAME_UNITS);
    s_poolCount = R16(s_game, GAME_POOL_COUNT);
    s_defs = RRef(s_game, GAME_DEFS);
    s_defCount = R32(s_game, GAME_DEF_COUNT);
    s_gridWidth = 0;
    s_gridHeight = 0;
    s_mask = 0;
    s_maskWords = 0;
    s_explored = 0;
    s_exploredSize = 0;
    viewer = s_viewer < VIEW_MAX_PLAYERS ? PlayerAt(s_viewer) : 0;
    if (viewer != 0) {
        s_gridWidth = R32(viewer, PLAYER_EXPLORED_WIDTH);
        s_gridHeight = R32(viewer, PLAYER_EXPLORED_HEIGHT);
        s_explored = RRef(viewer, PLAYER_EXPLORED);
        s_exploredSize = R32(viewer, PLAYER_EXPLORED_SIZE);
        if (s_explored == 0 || s_exploredSize > CELL_LIMIT)
            s_exploredSize = 0;
    }
    s_mask = RRef(s_game, GAME_VISIBILITY);
    width = R32(s_game, GAME_MAP_WIDTH);
    height = R32(s_game, GAME_MAP_HEIGHT);
    n = width * height;
    s_maskWords = n > CELL_LIMIT ? 0 : n / 4;

    ViewBegin(v);
    FillWorld(v);
    ViewSetGrid(v, s_gridWidth, s_gridHeight);
    FillPlayers(v);
    FillUnits(v);
    FillFeatures(v);
    FillProjectiles(v, report);
    FillDebris(v, report);
    FillExplodedPieces(v, report);
    FillMessages(v);
    FillSight(v);
}
