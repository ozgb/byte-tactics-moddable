#include <stdlib.h>
#include <string.h>
#include <memory>
#include "../state/game_rand.h"

#pragma pack(push, 1)
#include "../game/game.h"
#pragma pack(pop)

#include "../weapons/weapon_def.h"

#include "../util/vec3.h"

#include "../files/files_api.h"

#include "../util/tdf.h"
#include "../../interfaces/seams.h"
#include "../state/sim_globals.h"


class MeteorParams {
public:
    char name[0x20];
    int radius;
    float density;
    float duration;
    float interval;
    void LoadMeteorDefaults();
};

#include "../../link/globals.h"
typedef char SimView_g_meteorWeaponName[];
#undef g_meteorWeaponName
#define g_meteorWeaponName (*reinterpret_cast<SimView_g_meteorWeaponName*>(&Sim().g_meteorWeaponName))

int __cdecl ScaledSin(short angle, int scale);
int __cdecl ScaledCos(short angle, int scale);
#include "../game/data_files.h"
#include "../graphics/display.h"

// FUNCTION: 0x437cd0
void InitMeteors()
{
    g_meteorActive = 0;
    g_meteorNextStrikeTime = g_meteorSpawnInterval;
    g_meteorWeapon = g_seams.weapons->FindWeaponByName(g_meteorWeaponName);
    if (g_meteorWeapon == 0) {
        g_meteorWeapon = g_game->weapons;
        return;
    }
    if (!g_meteorWeapon->flags.b.meteor)
        g_meteorWeapon = g_game->weapons;
}

// FUNCTION: 0x437d30
void EmptyShutdownPreCleanup(void)
{
}

// FUNCTION: 0x437d40
void EnableMeteors()
{
    g_meteorsEnabled = 1;
}

// FUNCTION: 0x437d50
void DisableMeteors()
{
    g_meteorsEnabled = 0;
}

// FUNCTION: 0x437d60
void __stdcall SetMeteorParams(MeteorParams* p)
{
    strcpy(g_meteorWeaponName, p->name);
    g_meteorSpawnMagnitude = p->radius;
    g_meteorSpawnInterval = (int)(30.0f / p->density);
    g_meteorShowerDuration = (int)(p->duration * 30.0f);
    g_meteorScheduleGap = (int)(p->interval * 30.0f);
}

static inline Point16 MakePoint(int x, int y)
{
    Point16 p;
    p.x = x;
    p.y = y;
    return p;
}

static inline Point16 AddPoints(Point16 a, Point16 b)
{
    Point16 r;
    r.x = a.x + b.x;
    r.y = a.y + b.y;
    return r;
}

static inline void StartShower()
{
    g_meteorActive = 1;
    g_meteorStrikeEndTime = g_meteorShowerDuration + (unsigned int)g_game->ticks;
    g_meteorNextStrikeTime = g_meteorScheduleGap + g_meteorStrikeEndTime;
    g_meteorNextHitTime = (unsigned int)g_game->ticks;
    g_meteorTarget = MakePoint((int)((__int64)GameRand(RAND_STREAM_MAIN) * g_game->width / 0x8000),
                             (int)((__int64)GameRand(RAND_STREAM_MAIN) * g_game->height / 0x8000));
    g_meteorOrigin = AddPoints(MakePoint((int)((__int64)GameRand(RAND_STREAM_MAIN) * 30 / 0x8000) - 15,
                                       (int)((__int64)GameRand(RAND_STREAM_MAIN) * 10 / 0x8000) - 15),
                             g_meteorTarget);
}

// FUNCTION: 0x437de0
void UpdateMeteors()
{
    if (g_meteorNextStrikeTime <= (unsigned int)g_game->ticks) {
        StartShower();
        if (g_meteorsEnabled == 0)
            g_meteorActive = 0;
    }
    if (g_meteorActive != 0) {
        if (g_meteorNextHitTime <= (unsigned int)g_game->ticks) {
            g_meteorNextHitTime = g_meteorSpawnInterval + (unsigned int)g_game->ticks;
            Vec3 vel;
            vel.x = ((g_meteorTarget.x - g_meteorOrigin.x) << 20) / 90;
            vel.y = -15 << 16;
            vel.z = ((g_meteorTarget.y - g_meteorOrigin.y) << 20) / 90;
            int r = (int)((__int64)GameRand(RAND_STREAM_MAIN) * g_meteorSpawnMagnitude / 0x8000);
            int radius = r << 16;
            int angle = (int)((__int64)GameRand(RAND_STREAM_MAIN) * 0x10000 / 0x8000);
            Vec3 offset;
            offset.x = -ScaledSin(angle, radius);
            offset.y = 0;
            offset.z = -ScaledCos(angle, radius);
            Vec3 pos = offset;
            pos.x += g_meteorOrigin.x << 20;
            pos.y = -vel.y * 90;
            pos.z += g_meteorOrigin.y << 20;
            g_seams.weapons->SpawnProjectile(g_meteorWeapon, &pos, &vel, 1);
        }
        if (g_meteorStrikeEndTime <= (unsigned int)g_game->ticks)
            g_meteorActive = 0;
    }
}

// FUNCTION: 0x438070
void StartMeteorShower()
{
    g_meteorActive = 1;
    g_meteorStrikeEndTime = g_meteorShowerDuration + g_game->ticks;
    g_meteorNextStrikeTime = g_meteorScheduleGap + g_meteorStrikeEndTime;
    g_meteorNextHitTime = g_game->ticks;
    g_meteorTarget = MakePoint((int)((__int64)GameRand(RAND_STREAM_MAIN) * g_game->width / 0x8000),
                             (int)((__int64)GameRand(RAND_STREAM_MAIN) * g_game->height / 0x8000));
    g_meteorOrigin = AddPoints(MakePoint((int)((__int64)GameRand(RAND_STREAM_MAIN) * 30 / 0x8000) - 15,
                                       (int)((__int64)GameRand(RAND_STREAM_MAIN) * 10 / 0x8000) - 15),
                             g_meteorTarget);
}

// FUNCTION: 0x438180
void __stdcall SaveMeteors(HapiBank* file)
{
    file->OpenAccount("Meteor");
    file->SetIntegerItem("Enabled", g_meteorsEnabled);
    file->SetIntegerItem("Active", g_meteorActive);
    file->SetIntegerItem("Next Strike Time", g_meteorNextStrikeTime);
    file->SetIntegerItem("Time Strike Ends", g_meteorStrikeEndTime);
    file->SetIntegerItem("Next Hit Time", g_meteorNextHitTime);
    file->SetIntegerItem("Origin X", g_meteorOrigin.x);
    file->SetIntegerItem("Origin Z", g_meteorOrigin.y);
    file->SetIntegerItem("Target X", g_meteorTarget.x);
    file->SetIntegerItem("Target Z", g_meteorTarget.y);
}

// FUNCTION: 0x438250
void __stdcall LoadMeteors(HapiBank* file)
{
    file->OpenAccount("Meteor");
    g_meteorsEnabled = file->GetIntegerItem("Enabled", 0);
    g_meteorActive = file->GetIntegerItem("Active", 0);
    g_meteorNextStrikeTime = file->GetIntegerItem("Next Strike Time", 0);
    g_meteorStrikeEndTime = file->GetIntegerItem("Time Strike Ends", 0);
    g_meteorNextHitTime = file->GetIntegerItem("Next Hit Time", 0);
    g_meteorOrigin.x = file->GetIntegerItem("Origin X", 0);
    g_meteorOrigin.y = file->GetIntegerItem("Origin Z", 0);
    g_meteorTarget.x = file->GetIntegerItem("Target X", 0);
    g_meteorTarget.y = file->GetIntegerItem("Target Z", 0);
}

// FUNCTION: 0x438320
void MeteorParams::LoadMeteorDefaults()
{
    TdfFile parser;
    char path[256];
    BuildDataPath(path, "gamedata", "meteor", g_extTdf);
    if (parser.LoadFile(path)
        && parser.SelectRecord("Default")) {
        if (parser.current->GetFieldString((char*)this, "MeteorWeapon", 0x20, DAT_005119b8)) {
            radius = parser.current->GetFieldInt("MeteorRadius", 0);
            density = (float)parser.current->GetFieldDouble("MeteorDensity", 0.0);
            duration = (float)parser.current->GetFieldDouble("MeteorDuration", 0.0);
            float intervalTime = (float)parser.current->GetFieldDouble("MeteorInterval", 0.0);
            interval = intervalTime;
            if (radius != 0 && density != 0.0f && duration != 0.0f && intervalTime != 0.0f)
                return;
        }
        FatalError("Hey, hoser!  The default meteor shower data was bogus!");
    }
}
