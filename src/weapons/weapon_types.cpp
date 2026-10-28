#include <string.h>
#include "../state/block.h"
#include <vector>

#include "../util/tdf.h"
#include "weapon_def.h"
#include "damage_table.h"

#define PI 3.14159265358979

#include "../util/string_ref.h"

static inline bool operator==(const StringRef& a, const StringRef& b) {
    return strcmp(a.ptr, b.ptr) == 0;
}

static inline bool Ne(const StringRef& a, const StringRef& b) {
    return !(a == b);
}

static inline bool Less(const char* a, const char* b) {
    return _strcmpi(a, b) < 0;
}

#include "../map/los_tables.h"

inline DamageTableEntry* Table_00499cd0::LowerBound(const char* key) {
    DamageTableEntry* first = v.begin();
    DamageTableEntry* last = v.end();
    while (first != last) {
        DamageTableEntry* mid = first + (last - first) / 2;
        if (Less(mid->name.ptr, key))
            first = mid + 1;
        else
            last = mid;
    }
    return first;
}

#include "weapons.h"

#pragma pack(push, 1)
#include "../game/game.h"
#include "../state/sim_globals.h"
#pragma pack(pop)

#include "../../link/globals.h"

#include "../game/data_files.h"
#include "../graphics/display.h"
#include "../graphics/pcx.h"
void __stdcall MirrorObject(void* p);
void __stdcall BindModelTextures(void* a, char* b);
#include "../game/data_files.h"
void* __stdcall FindGafEntry(void* a, char* b);

#include "../files/files_api.h"
#include "weapon_types.h"
#include "../game/cd_check.h"
extern void __cdecl operator delete(void*);
#include "../graphics/object_pool.h"

// FUNCTION: 0x42e310
void LoadWeaponTypes()
{
    int n = 0;
    for (int i = 0; i < 0x100; i++) {
        WeaponDef* p = &g_game->weapons[i];
        p->index = n++;
        p->name[0] = 0;
    }

    char path[256];
    std::vector<StringRef> files;
    ListDirectory("Weapons\\*.tdf", 0, &files);

    for (StringRef* p = files.begin(); p < files.end(); p++) {
        TdfFile parser;
        BuildDataPath(path, "Weapons", p->ptr, "TDF");
        if (parser.LoadFile(path)
            && (parser.flags || UsesDataArchives() == 0)) {
            int i = 0;
            while (1) {
                parser.ResetCurrentRecord();
                if (!parser.SelectRecordAt(i))
                    break;
                LoadWeaponType(parser.current);
                i++;
            }
        }
    }
}

// FUNCTION: 0x42e440
void __stdcall LoadWeaponType(TdfRecord* parser) {
    char* id = (char*)parser->GetRecordName();
    WeaponDef* w = &g_game->weapons[parser->GetFieldInt("ID", -1)];
    strcpy(w->name, id);
    parser->GetFieldString(w->description, "name", 0x40, DAT_005119b8);

    w->speed =
        (int)(parser->GetFieldDouble("weaponvelocity", 0.0) * 2184.5333333333333);
    w->startSpeed =
        (int)(parser->GetFieldDouble("startvelocity", 0.0) * 2184.5333333333333);
    w->acceleration =
        (int)(parser->GetFieldDouble("weaponacceleration", 0.0) *
              72.81777777777778);
    w->range = parser->GetFieldInt("range", 0x7fff);
    w->coverage = parser->GetFieldInt("coverage", 0);
    w->reloadTime = (unsigned short)(parser->GetFieldDouble("reloadtime", 0.0) * 30.0);
    w->energyPerShot = (float)parser->GetFieldDouble("energypershot", 0.0);
    w->metalPerShot = (float)parser->GetFieldDouble("metalpershot", 0.0);
    w->areaOfEffect = (unsigned short)parser->GetFieldInt("areaofeffect", 0);
    w->edgeDamage = (float)parser->GetFieldDouble("edgeeffectiveness", 0.0);
    w->lifetime = (unsigned short)(parser->GetFieldDouble("weapontimer", 0.0) * 30.0);
    w->flags.b.noAutoRange = parser->GetFieldInt("noautorange", 0);
    w->turnRate =
        (unsigned short)(parser->GetFieldDouble("turnrate", 0.0) * 0.03333333333333333);
    w->burst = (unsigned short)parser->GetFieldInt("burst", 0);
    w->burstRate = (unsigned short)(parser->GetFieldDouble("burstrate", 0.0) * 30.0);
    w->spread = (unsigned short)parser->GetFieldInt("sprayangle", 0);
    w->duration = (unsigned short)(parser->GetFieldDouble("duration", 0.0) * 30.0);
    w->lifeRand = (unsigned short)(parser->GetFieldDouble("randomdecay", 0.0) * 30.0);
    w->smokeRate = (unsigned short)(parser->GetFieldDouble("smokedelay", 0.0) * 30.0);
    w->flightTime = (unsigned short)(parser->GetFieldDouble("flighttime", 0.0) * 30.0);
    w->holdTime = (unsigned short)(parser->GetFieldDouble("holdtime", 0.0) * 30.0);
    w->pitch =
        parser->GetFieldDouble("minbarrelangle", -11.25) * (PI / 180);
    w->fireStarter = (unsigned char)parser->GetFieldInt("firestarter", 0);
    w->shotKind = (unsigned char)parser->GetFieldInt("rendertype", 0);
    w->colour = (unsigned char)parser->GetFieldInt("color", 0);
    w->colour2 = (unsigned char)parser->GetFieldInt("color2", 0);
    w->flags.b.soundTrigger = parser->GetFieldInt("soundtrigger", 0);
    w->flags.b.guidance = parser->GetFieldInt("guidance", 0);
    w->flags.b.tracks = parser->GetFieldInt("tracks", 0);
    w->flags.b.lineOfSight = parser->GetFieldInt("lineofsight", 0);
    int ballistic = parser->GetFieldInt("ballistic", 0);
    w->flags.b.ballistic = ballistic;
    w->flags.b.unitsOnly = parser->GetFieldInt("unitsonly", 0);
    w->flags.b.groundBounce = parser->GetFieldInt("groundbounce", 0);
    w->flags.b.waterWeapon = parser->GetFieldInt("waterweapon", 0);
    w->flags.b.toAirWeapon = parser->GetFieldInt("toairweapon", 0);
    w->flags.b.smokeTrail = parser->GetFieldInt("smoketrail", 0);
    w->flags.b.turret = parser->GetFieldInt("turret", 0);
    w->flags.b.selfProp = parser->GetFieldInt("selfprop", 0);
    w->flags.b.propeller = parser->GetFieldInt("propeller", 0);
    w->flags.b.noExplode = parser->GetFieldInt("noexplode", 0);
    w->flags.b.burnBlow = parser->GetFieldInt("burnblow", 0);
    w->flags.b.twoPhase = parser->GetFieldInt("twophase", 0);
    w->flags.b.cruise = parser->GetFieldInt("cruise", 0);
    w->flags.b.commandFire = parser->GetFieldInt("commandfire", 0);
    w->flags.b.stockpile = parser->GetFieldInt("stockpile", 0);
    w->flags.b.targetable = parser->GetFieldInt("targetable", 0);
    w->flags.b.interceptor = parser->GetFieldInt("interceptor", 0);
    w->flags.b.beamWeapon = parser->GetFieldInt("beamweapon", 0);
    w->flags.b.shellWeapon = parser->GetFieldInt("shellweapon", 0);
    int dropped = parser->GetFieldInt("dropped", 0);
    w->flags.b.dropped = dropped;
    w->flags.b.vLaunch = parser->GetFieldInt("vlaunch", 0);
    w->flags.b.meteor = parser->GetFieldInt("meteor", 0);
    w->flags.b.noRadar = parser->GetFieldInt("noradar", 0);
    w->flags.b.paralyzer = parser->GetFieldInt("paralyzer", 0);
    w->flags.b.startSmoke = parser->GetFieldInt("startsmoke", 0);
    w->flags.b.endSmoke = parser->GetFieldInt("endsmoke", 0);
    w->accuracy = (short)parser->GetFieldInt("accuracy", 0);
    w->tolerance = (unsigned short)parser->GetFieldInt("tolerance", 0);
    w->pitchTolerance = (unsigned short)parser->GetFieldInt("pitchtolerance", 0);
    w->shakeMagnitude = parser->GetFieldInt("shakemagnitude", 0);
    w->shakeDuration = (int)(parser->GetFieldDouble("shakeduration", 0.0) * 30.0);

    char model[0x100];
    if (parser->GetFieldString(model, "model", 0x100, DAT_005119b8) != 0) {
        unsigned char i;
        unsigned char count = w->index;
        for (i = 0; i < count; i++) {
            if (_strcmpi(model, g_game->weapons[i].modelName) == 0) {
                g_game->weapons[count].modelName[0] = 0;
                g_game->weapons[count].model = g_game->weapons[i].model;
                goto model_done;
            }
        }
        char path[0x100];
        BuildDataPath(path, "objects3d", model, "3DO");
        void* h = Load3do(path);
        if (h == 0)
            FatalError(path);
        MirrorObject(h);
        BindModelTextures(h, model);
        g_game->weapons[count].model = h;
        strcpy(g_game->weapons[count].modelName, model);
    } else {
        w->model = 0;
    }
model_done:
    w->explosionSeq = 0;
    char gaf[0x100];
    if (parser->GetFieldString(gaf, "explosiongaf", 0x100, DAT_005119b8) != 0 &&
        parser->GetFieldString(model, "explosionart", 0x100, DAT_005119b8) != 0) {
        void* a = LoadAnimGaf(gaf);
        void* r = FindGafEntry(a, model);
        *(unsigned char*)((char*)r + 2) = 0;
        w->explosionSeq = r;
    }
    w->splash = 0;
    if (*(int*)((char*)g_game->net.get() + 0xd44) != 0) {
        if (parser->GetFieldString(gaf, "lavaexplosiongaf", 0x100, DAT_005119b8) != 0 &&
            parser->GetFieldString(model, "lavaexplosionart", 0x100, DAT_005119b8) != 0) {
            void* a = LoadAnimGaf(gaf);
            void* r = FindGafEntry(a, model);
            *(unsigned char*)((char*)r + 2) = 0;
            w->splash = r;
        }
    } else {
        if (parser->GetFieldString(gaf, "waterexplosiongaf", 0x100, DAT_005119b8) != 0 &&
            parser->GetFieldString(model, "waterexplosionart", 0x100, DAT_005119b8) != 0) {
            void* a = LoadAnimGaf(gaf);
            void* r = FindGafEntry(a, model);
            *(unsigned char*)((char*)r + 2) = 0;
            w->splash = r;
        }
    }
    if (parser->GetFieldString(model, "soundstart", 0x100, DAT_005119b8) != 0) {
        w->sound = (unsigned short)LoadSoundByName(0, model);
    } else {
        w->sound = 0xffff;
    }
    if (parser->GetFieldString(model, "soundhit", 0x100, DAT_005119b8) != 0) {
        w->soundHit = (unsigned short)LoadSoundByName(0, model);
    } else {
        w->soundHit = 0xffff;
    }
    if (parser->GetFieldString(model, "soundwater", 0x100, DAT_005119b8) != 0) {
        w->soundWater = (unsigned short)LoadSoundByName(0, model);
    } else {
        w->soundWater = 0xffff;
    }
    TdfRecord* damage = parser->FindSubRecord("DAMAGE");
    if (damage != 0) {
        w->damage = (unsigned short)damage->GetFieldInt("default", 0);
        int index = 0;
        char* key = damage->GetFieldName(index);
        while (key) {
            if (_strcmpi(key, "default") != 0) {
                int value = damage->GetFieldInt(key, 0);
                if (!w->table)
                    w->table = new (kSim) Table_00499cd0;
                StringRef name(key);
                Table_00499cd0* m = w->table;
                DamageTableEntry* e = m->LowerBound(name.ptr);
                int* r;
                if (e == m->v.end() || Ne(e->name, name)) {
                    e = m->v.insert(e, DamageTableEntry(name, 0));
                    r = &e->value;
                } else {
                    r = &e->value;
                }
                *r = value;
            }
            key = damage->GetFieldName(++index);
        }
    } else {
        w->damage = 0;
    }
    SetWeaponFireHandler(w);
}

// FUNCTION: 0x42f3a0
void FreeWeaponTypes()
{
    int i = 0;
    while (i < 256) {
        WeaponDef* o = &g_game->weapons[i];
        if (strlen(o->modelName) != 0) {
            GameFreeThunk(o->model);
            o->model = 0;
            o->modelName[0] = 0;
        }
        if (o->table) {
            delete o->table;
            o->table = 0;
        }
        i++;
    }
}
