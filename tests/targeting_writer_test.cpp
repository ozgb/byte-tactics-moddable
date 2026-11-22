#include <stdio.h>
#include <string.h>
#include <vector>

#include "../src/state/block.h"
#include "../interfaces/seams.h"
#include "../interfaces/services.h"
#include "../interfaces/targeting.h"
#include "../interfaces/los_radar.h"
#include "../interfaces/units.h"
#include "../interfaces/scripts.h"
#include "../interfaces/weapons.h"
#include "../src/state/sim_globals.h"
#include "../src/units/unit_type_set.h"
#include "../src/units/unit_def.h"
#include "../src/util/vec3.h"
#pragma pack(push, 1)
#include "../src/game/unit_resources.h"
#include "../src/network/player.h"
#include "../src/units/unit.h"
#include "../src/game/game.h"
#pragma pack(pop)
#include "../src/game/targeting_events.h"
#include "../src/units/unit_targets.h"

Game* g_game;
Seams g_seams;
SimGlobals* g_simGlobals = 0;
void (*g_outOfMemoryHandler)() = 0;

static int g_failures;

static void Check(bool ok, const char* what)
{
    if (!ok) {
        printf("FAIL: %s\n", what);
        g_failures++;
    }
}

void* GetAllocLock()
{
    static void* lock;
    if (lock == 0)
        lock = host_mutex_create();
    return lock;
}

int __stdcall HarnessTestMode()
{
    return 0;
}

extern "C" void HOST_CALL host_fatal(void*, const char* title, const char* text)
{
    printf("FAIL: host_fatal: %s: %s\n", title, text);
    g_failures++;
}

void __stdcall HarnessTargetingEvent(const TargetingEventHeader*, unsigned int, int) {}
void __stdcall SchedPublish(int, const void*, unsigned int) {}
void __stdcall FatalError(char* text)
{
    printf("FAIL: FatalError: %s\n", text);
    g_failures++;
}
int __stdcall RandomInt(int n)
{
    return n > 0 ? n - 1 : 0;
}

char g_hitByWeaponScriptName[12];
char g_takeDamageScriptName[12];
char g_paralyzeMissionName[12];

extern "C" double __cdecl bx_sqrt(double)
{
    return 0;
}
extern "C" unsigned int HOST_CALL host_time_ms(void)
{
    return 0;
}
SimGlobals& __stdcall SimCreate()
{
    static char storage[sizeof(SimGlobals)];
    return *(SimGlobals*)storage;
}
void __stdcall RecordSimEvent(SimEventKind, unsigned int, const unsigned int*, unsigned int) {}
char* __stdcall Translate(const char* key)
{
    return (char*)key;
}
void* __cdecl GameAllocIgnoreTag(const char*, unsigned int)
{
    return 0;
}
int __cdecl ScaledSin(int, int)
{
    return 0;
}
int __cdecl ScaledCos(int, int)
{
    return 0;
}
int __cdecl AngleOfVector(int, int)
{
    return 0;
}
struct XzPoint;
void __cdecl RotatePoint2D(unsigned short, XzPoint*) {}

enum {
    kTicks = 60,
    kTamperTick = 25,
    kLostTick = 25,
    kUnits = 8,
    kShooter = 1,
    kEnemies = 3,
    kFirstEnemy = 2
};

static Unit* s_units;
static std::vector<Unit*> s_candidates;
static int s_tick;
static bool s_tamper;
static int s_tampered;

static Unit* UnitById(int id)
{
    return &s_units[id];
}

static unsigned int HandleOf(Unit* unit)
{
    return Ref<Unit>(unit).offset;
}

static int __stdcall GetVisibleEnemies(int, const Vec3*, int, int, Ref<Unit>* out, int capacity)
{
    int count = (int)s_candidates.size();
    for (int i = 0; i < count && i < capacity; i++)
        out[i] = s_candidates[i];
    return count;
}

static void __stdcall GetSweetSpot(Unit* unit, Vec3* out)
{
    *out = unit->pos;
}

static int __stdcall GetGroundHeight(Vec3*)
{
    return 0;
}

static Unit* __stdcall TamperedFindWeaponTarget(Unit* unit, unsigned char weapon, int useRange)
{
    Unit* answer = FindWeaponTarget(unit, weapon, useRange);
    if (s_tamper && s_tick == kTamperTick) {
        s_tampered = s_tick;
        return UnitById(kFirstEnemy + 1);
    }
    return answer;
}

static unsigned long long Fnv(unsigned long long h, const void* p, unsigned int n)
{
    const unsigned char* b = (const unsigned char*)p;
    for (unsigned int i = 0; i < n; i++)
        h = (h ^ b[i]) * 0x100000001b3ull;
    return h;
}

struct Run {
    unsigned long long units[kTicks];
    unsigned long long projectiles[kTicks];
    int tampered;
    int fired;
    int answers[kTicks];
};

static void Restart()
{
    for (int i = 0; i < kUnits; i++)
        for (int w = 0; w < 3; w++) {
            s_units[i].weapons[w].aimTargetXOrUnitId = 0;
            s_units[i].weapons[w].aimTargetZOrUnitSentinel = (short)0x8000;
        }
}

static void Setup()
{
    SimBlockInit();
    g_game = (Game*)SimAlloc(sizeof(Game));
    memset(g_game, 0, sizeof(Game));
    s_units = (Unit*)SimAlloc(kUnits * sizeof(Unit));
    memset(s_units, 0, kUnits * sizeof(Unit));
    g_game->units = s_units;
    g_game->poolCount = kUnits;

    Player* players = (Player*)SimAlloc(2 * sizeof(Player));
    memset(players, 0, 2 * sizeof(Player));
    players[0].active = 1;
    players[1].active = 1;

    UnitDef* shooterDef = (UnitDef*)SimAlloc(sizeof(UnitDef));
    memset(shooterDef, 0, sizeof(UnitDef));
    UnitDef* enemyDef = (UnitDef*)SimAlloc(sizeof(UnitDef));
    memset(enemyDef, 0, sizeof(UnitDef));
    UnitTypeSet* none = (UnitTypeSet*)SimAlloc(sizeof(UnitTypeSet));
    memset(none, 0, sizeof(UnitTypeSet));
    shooterDef->flags1 = 0x10000000;
    shooterDef->categories = none;
    for (int w = 0; w < 3; w++)
        shooterDef->weaponCategories[w] = none;
    enemyDef->flags1 = 0x8000;

    void* weaponDef = SimAlloc(0x200);
    memset(weaponDef, 0, 0x200);

    for (int i = kShooter; i < kFirstEnemy + kEnemies; i++) {
        Unit* u = &s_units[i];
        u->id = (unsigned short)i;
        u->unitDefIndex = (unsigned short)i;
        u->flags = 0x10000000;
        u->def = i == kShooter ? shooterDef : enemyDef;
        u->player = &players[i == kShooter ? 0 : 1];
        u->playerIndex = (unsigned char)(i == kShooter ? 0 : 1);
        for (int w = 0; w < 3; w++)
            u->weapons[w].weapon = (WeaponDef*)weaponDef;
    }
    s_units[kShooter].weapons[0].flags = 2;
    for (int e = 0; e < kEnemies; e++) {
        Unit* enemy = UnitById(kFirstEnemy + e);
        enemy->pos.x = (10 + 10 * e) << 16;
        enemy->pos.z = (5 + 7 * e) << 16;
        s_candidates.push_back(enemy);
    }
    Restart();
}

static void Play(Run* run, bool tamper)
{
    Restart();
    s_tamper = tamper;
    s_tampered = -1;
    memset(run, 0, sizeof *run);
    Unit* shooter = UnitById(kShooter);
    unsigned long long shots = 0xcbf29ce484222325ull;
    for (s_tick = 0; s_tick < kTicks; s_tick++) {
        const TargetingSeam& seam = *g_seams.targeting;
        if (s_tick == kLostTick) {
            TargetingEventHeader lost = TargetingHeaderOf(TARGETING_CLEAR, shooter, 0);
            seam.Apply(&lost, sizeof lost);
        }
        if (seam.GetWeaponTargetUnit(shooter, 0) == 0) {
            Unit* pick = seam.FindWeaponTarget(shooter, 0, 0);
            run->answers[s_tick] = pick ? pick->id : 0;
            if (pick) {
                TargetingSetUnit aim;
                aim.h = TargetingHeaderOf(TARGETING_SET_UNIT, shooter, 0);
                aim.target = HandleOf(pick);
                seam.Apply(&aim.h, sizeof aim);
            }
        }
        Vec3 pos;
        if (seam.GetWeaponTargetPos(shooter, &pos, 0)) {
            shots = Fnv(shots, &pos, sizeof pos);
            run->fired++;
        }
        run->projectiles[s_tick] = shots;
        unsigned long long h = 0xcbf29ce484222325ull;
        for (int i = 0; i < kUnits; i++)
            h = Fnv(h, s_units[i].weapons, sizeof s_units[i].weapons);
        run->units[s_tick] = h;
    }
    run->tampered = s_tampered;
}

static void CheckRejected()
{
    Restart();
    Unit* shooter = UnitById(kShooter);
    TargetingSetUnit aim;
    aim.h = TargetingHeaderOf(TARGETING_SET_UNIT, shooter, 0);
    aim.target = HandleOf(UnitById(kFirstEnemy));
    Check(ApplyTargetingEvent(&aim.h, sizeof aim) == 1, "a valid SET_UNIT is applied");
    std::vector<char> before((char*)s_units, (char*)(s_units + kUnits));

    TargetingEventHeader noUnit = TargetingHeaderOf(TARGETING_CLEAR, shooter, 0);
    noUnit.unit = 0;
    TargetingSetUnit noTarget;
    memset(&noTarget, 0, sizeof noTarget);
    noTarget.h = TargetingHeaderOf(TARGETING_SET_UNIT, shooter, 1);
    TargetingSetPos high;
    memset(&high, 0, sizeof high);
    high.h = TargetingHeaderOf(TARGETING_SET_POS, shooter, 3);
    TargetingEventHeader low = TargetingHeaderOf(TARGETING_RESET, shooter, -1);
    TargetingEventHeader clearHigh = TargetingHeaderOf(TARGETING_CLEAR, shooter, 3);
    TargetingLoad loadHigh;
    memset(&loadHigh, 0, sizeof loadHigh);
    loadHigh.h = TargetingHeaderOf(TARGETING_LOAD, shooter, 3);
    const TargetingEventHeader* rejected[] = { &noUnit, &noTarget.h, &high.h, &low, &clearHigh, &loadHigh.h };
    const unsigned int sizes[] = { sizeof noUnit, sizeof noTarget, sizeof high, sizeof low, sizeof clearHigh, sizeof loadHigh };
    for (int i = 0; i < 6; i++)
        Check(ApplyTargetingEvent(rejected[i], sizes[i]) == 0, "an event with a null unit, a null target or a weapon outside 0 to 2 has outcome 0");
    Check(ApplyTargetingEvent(&aim.h, sizeof(TargetingEventHeader)) == 0, "an event smaller than its payload has outcome 0");
    Check(memcmp(&before[0], s_units, before.size()) == 0, "a rejected event changes no unit");
}

static void CheckLoad()
{
    Restart();
    Unit* shooter = UnitById(kShooter);
    shooter->netDirtyFlags = 0x7c00;
    std::vector<char> before((char*)s_units, (char*)(s_units + kUnits));

    TargetingLoad unitTarget;
    memset(&unitTarget, 0, sizeof unitTarget);
    unitTarget.h = TargetingHeaderOf(TARGETING_LOAD, shooter, 0);
    unitTarget.xOrUnit = kFirstEnemy;
    unitTarget.zOrSentinel = (short)0x8000;
    TargetingLoad ground;
    memset(&ground, 0, sizeof ground);
    ground.h = TargetingHeaderOf(TARGETING_LOAD, shooter, 1);
    ground.xOrUnit = 12;
    ground.zOrSentinel = -7;
    TargetingLoad none;
    memset(&none, 0, sizeof none);
    none.h = TargetingHeaderOf(TARGETING_LOAD, shooter, 2);
    none.zOrSentinel = (short)0x8000;
    Check(ApplyTargetingEvent(&unitTarget.h, sizeof unitTarget) == 1 && ApplyTargetingEvent(&ground.h, sizeof ground) == 1 &&
          ApplyTargetingEvent(&none.h, sizeof none) == 1, "LOAD has outcome 1");

    UnitWeaponSlot* slots = shooter->weapons;
    Check(slots[0].aimTargetXOrUnitId == kFirstEnemy && slots[0].aimTargetZOrUnitSentinel == (short)0x8000, "LOAD keeps a target unit's pair as saved");
    Check(slots[1].aimTargetXOrUnitId == 12 && slots[1].aimTargetZOrUnitSentinel == -7, "LOAD keeps a ground point's pair as saved");
    Check(slots[2].aimTargetXOrUnitId == 0 && slots[2].aimTargetZOrUnitSentinel == (short)0x8000, "LOAD keeps an empty pair as saved");
    Check(GetWeaponTargetUnit(shooter, 0) == UnitById(kFirstEnemy), "a loaded target unit is the weapon's target");
    Check(shooter->netDirtyFlags == 0x7c00, "LOAD leaves the dirty flags");

    Restart();
    Check(memcmp(&before[0], s_units, before.size()) == 0, "LOAD writes the target pairs and nothing else");
}

int main()
{
    Setup();
    TargetingSeam seam;
    memset(&seam, 0, sizeof seam);
    seam.Apply = ApplyTargetingEvent;
    seam.GetWeaponTargetUnit = GetWeaponTargetUnit;
    seam.GetWeaponTargetPos = GetWeaponTargetPos;
    seam.FindWeaponTarget = TamperedFindWeaponTarget;
    g_seams.targeting = &seam;
    static LosRadarSeam losRadar;
    memset(&losRadar, 0, sizeof losRadar);
    losRadar.GetVisibleEnemiesInRadius = GetVisibleEnemies;
    g_seams.los_radar = &losRadar;
    static MapSeam map;
    memset(&map, 0, sizeof map);
    map.GetGroundHeight = GetGroundHeight;
    g_seams.map = &map;
    static ScriptsSeam scripts;
    memset(&scripts, 0, sizeof scripts);
    scripts.GetSweetSpot = GetSweetSpot;
    g_seams.scripts = &scripts;

    static Run plain, again, tampered;
    Play(&plain, false);
    Play(&again, false);
    Play(&tampered, true);

    Check(plain.fired == kTicks, "the weapon has a target to fire at every tick");
    Check(plain.answers[0] == kFirstEnemy, "the default picks the nearest enemy");
    Check(plain.units[0] != plain.units[kTicks - 1] || plain.projectiles[0] != plain.projectiles[1], "the weapon's target and the projectiles are hashed");
    Check(memcmp(&plain, &again, sizeof plain) == 0, "two runs of the default give the same hashes at every tick");
    Check(tampered.tampered == kTamperTick, "the tampered module answers differently at the tampered tick");
    int answersDiffering = 0;
    for (int t = 0; t < kTicks; t++)
        if (plain.answers[t] != tampered.answers[t])
            answersDiffering++;
    Check(answersDiffering == 1 && plain.answers[kTamperTick] != tampered.answers[kTamperTick], "the tampered module's answer differs at the tampered tick only");

    int firstUnit = -1, firstShot = -1, differingUnit = 0, differingShot = 0;
    for (int t = 0; t < kTicks; t++) {
        if (plain.units[t] != tampered.units[t]) {
            if (firstUnit < 0)
                firstUnit = t;
            differingUnit++;
        }
        if (plain.projectiles[t] != tampered.projectiles[t]) {
            if (firstShot < 0)
                firstShot = t;
            differingShot++;
        }
    }
    Check(firstUnit == kTamperTick, "the unit's weapon target differs first at the tampered tick");
    Check(firstShot == kTamperTick, "the projectiles differ first at the tampered tick");
    Check(differingUnit == kTicks - kTamperTick, "the unit's weapon target differs at every tick from the tampered tick on");
    Check(differingShot == kTicks - kTamperTick, "the projectiles differ at every tick from the tampered tick on");
    printf("tampered tick %d, first differing tick %d, %d differing ticks\n", tampered.tampered, firstUnit, differingUnit);

    CheckRejected();
    CheckLoad();

    printf(g_failures ? "FAILED\n" : "targeting_writer_test: all passed\n");
    return g_failures != 0;
}
