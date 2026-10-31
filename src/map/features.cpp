#include "win_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include "../state/block.h"
#include "../state/game_rand.h"

#include "../util/tdf.h"
#include "../files/files_api.h"
#include "../../interfaces/ref.h"
#include "../util/rot16.h"
#include "tnt_info.h"
#include "feature_anim_pool.h"

#pragma pack(push, 1)

#include "feature.h"

#include "../util/vec3.h"

static inline int Vec3NonZero(Vec3* v)
{
    return v->x || v->z || v->y;
}

static inline Vec3 FeatureVec3(int ax, int ay, int az)
{
    Vec3 v;
    v.x = ax;
    v.y = ay;
    v.z = az;
    return v;
}

union Coord {
    int value;
    Fixed f;
};

struct SmokePos {
    Coord x, y, z;
};

#include "../graphics/gaf_frame.h"

struct Feature;
struct SpotState;

#include "spot.h"

#include "cell.h"

#include "../network/player.h"
#include "../network/player_info.h"

#include "../units/unit.h"
#include "../weapons/weapon_def.h"

struct FeatureSite {
    char unknown_0[0x28];
    short x;
    short y;
    unsigned short entry;
};

struct FeatureName {
    char name[0x80];
};

struct Normal {
    short x;
    short y;
    unsigned short feature;
    unsigned short spot;
};

struct AnimRecord {
    short x;
    short y;
    unsigned short feature;
    unsigned short damage;
    unsigned char frame;
    unsigned char anim : 4;
    unsigned char animHi : 4;
};

struct Model {
    short x;
    short y;
    unsigned short feature;
    unsigned short damage;
    Vec3 pos;
    Rot16 rot;
};

#include "../map/mission.h"

#include "../game/game.h"
#include "../state/sim_globals.h"
#pragma pack(pop)

#include "string_ref_view.h"

struct Elem {
    StringRefView name;
};

#include "../../link/globals.h"

typedef std::vector<TdfFile*> FeatureList;
static FeatureList* s_featureTdfParsers;

#include "../graphics/object_pool.h"
#include "../game/data_files.h"
#include "../graphics/display.h"
void* __cdecl GameReallocTagged(void* p, const char* name, unsigned int size);
Gaf* __stdcall LoadGaf(char* path);
void __stdcall FreeObjectState(void* obj);
AnimSrc* __stdcall FindGafEntry(void* list, const char* name);
void __stdcall InitGafSequence(Handle* ref, AnimSrc* src, int index);
Spot* __stdcall PlaceFeature(Cell* cell, unsigned short feature, Vec3* pos, Rot16* rot,
                             unsigned char owner);
#include "features.h"
#include "../util/math.h"
void __stdcall PlaySoundAtByName(char* name, Vec3* pos, int param_3);
#include "../network/net_game.h"
int GetHostDpid();
int __stdcall BroadcastPacket(int player, void* data, int size);
int __stdcall StepGafSequence(Handle* anim);
GafFrame* __stdcall GetGafSequenceFrame(Handle* anim);
void __stdcall EmitWhiteSmoke(SmokePos* pos, short index);
#include "../graphics/model_render.h"
#include "../graphics/particles.h"

// FUNCTION: 0x421e60
unsigned short __stdcall GetCellFeature(Cell* cell)
{
    if (cell->feature >= 0xfffb) {
        if (cell->feature == 0xfffe)
            return (cell - (cell->offsetY * g_game->width + cell->offsetX))->feature;
        return 0xffff;
    }
    return cell->feature;
}

#include "../../interfaces/seams.h"

// FUNCTION: 0x421eb0
Vec3 __stdcall GetFootprintCentre(Point16* cell, Feature* def)
{
    Point16 f = def->footprint;
    Point16 c = *cell;
    Vec3 p;
    p.x = (f.x + c.x * 2) << 19;
    p.z = (f.y + c.y * 2) << 19;
    p.y = g_seams.units->GetGroundHeight(&p) << 16;
    return p;
}

// FUNCTION: 0x421f20
void __stdcall InitFeatureAnimPool(TntInfo* info)
{
    FeatureAnimPool* m = &g_game->anim;

    m->anim = (FeatureAnimEntry*)SimAllocTagged("FEATURE ANIM DATA", 0x18000);
    memset(m->anim, 0, 0x18000);
    m->activeHead = -1;
    m->idleHead = -1;
    m->freeHead = 0;
    for (int i = 0; i < 0x800; i++) {
        m->anim[i].next = i + 1;
        m->anim[i].prev = i - 1;
    }
    m->anim[0].prev = -1;
    m->anim[0x7ff].next = -1;

    m->featureUnit = (FeaturePseudoUnit*)SimAllocTagged("Feature Unit", 0x118);
    memset(m->featureUnit, 0, 0x118);
    m->featureUnit->flags_110 |= 0x200;
    m->featureUnit->flags_110 |= 0x20000000;
    m->featureUnit->unitDefIndex = 0;
    m->featureUnit->flags_114 |= 1;
    m->featureUnit->def = g_game->defs;
    m->names = nullptr;
    m->nameCount = 0;

    for (int j = 0; j < info->featureCount; j++) {
        LoadFeatureType(info->featureNames[j].name);
    }
}

// FUNCTION: 0x422040
void StampFeatureMetal(void)
{
    Cell* c = g_game->cells;
    for (int i = 0; i < g_game->width * g_game->height; i++, c++) {
        if (c->feature < 0xfffb) {
            Feature* f = &g_game->features[c->feature];
            if (f->metal != 0.0f && (f->flags16 & 0x200)) {
                int x0 = i % g_game->width;
                int y0 = i / g_game->width;
                for (int y = y0; y < y0 + f->footprintZ; y++) {
                    for (int x = x0; x < x0 + f->footprintX; x++) {
                        Cell* cc = g_seams.los_radar->GetMapCell(x, y);
                        if (cc != 0)
                            cc->metal = (char)f->metal;
                    }
                }
            }
        }
    }
}

// FUNCTION: 0x422170
void FreeFeaturePool()
{
    int i;
    for (i = 0; i <= 1; i++) {
        int idx = i ? g_game->pool.usedHead : g_game->pool.restHead;
        while (idx != -1) {
            Spot* e = &g_game->pool.entries[idx];
            if (!(g_game->features[e->feature].flags & 1))
                FreeObjectState(e->state);
            idx = e->prev;
        }
    }

    GameFreeThunk(g_game->pool.featureUnit);
    GameFreeThunk(g_game->pool.entries);
    g_game->pool.featureUnit = 0;
    g_game->pool.entries = 0;

    Feature* t = g_game->features;
    for (i = 0; i < g_game->featureCount; i++, t++) {
        if (t->flags & 1) {
            if (t->anims) {
                GameFreeThunk(t->anims);
                t->anims = 0;
            }
        } else {
            if (t->object) {
                GameFreeThunk(t->object);
                t->object = 0;
            }
        }
    }

    GameFreeThunk(g_game->features);
    g_game->features = 0;
    g_game->featureCount = 0;
}

// FUNCTION: 0x4222b0
void* __stdcall FindOptionalGafEntry(void* list, int unused, char* name)
{
    if (strlen(name) == 0) {
        return 0;
    }
    return FindGafEntry(list, name);
}

// FUNCTION: 0x4222e0
void __stdcall LoadFeatureFileList()
{
    s_featureTdfParsers = new std::vector<TdfFile*>;
    std::vector<Elem> list;
    FindFilesRecursive("features", "*.tdf", (std::vector<StringRef>*)&list, -1, 1);
    for (std::vector<Elem>::iterator it = list.begin(); it < list.end(); it++) {
        TdfFile* obj = new TdfFile;
        if (obj->LoadFile(it->name.data)) {
            s_featureTdfParsers->push_back(obj);
        } else {
            delete obj;
        }
    }
}

// FUNCTION: 0x4223e0
void FreeFeatureFileList()
{
    for (TdfFile** p = s_featureTdfParsers->begin(); p < s_featureTdfParsers->end(); p++)
        delete *p;
    delete s_featureTdfParsers;
    s_featureTdfParsers = 0;
}

// FUNCTION: 0x422460
TdfFile* __stdcall FindFeatureFile(char* name)
{
    for (TdfFile** p = s_featureTdfParsers->begin(); p < s_featureTdfParsers->end(); p++) {
        (*p)->ResetCurrentRecord();
        if (((TdfFile*)*p)->SelectRecord(name))
            return *p;
    }
    return 0;
}

static inline void LoadFeatureSeq(TdfRecord* rec, char* key, char* seqname, Gaf* anims, AnimSrc** out)
{
    int ok = rec->GetFieldString(seqname, key, 0x100, DAT_005119b8);
    if (ok)
        *out = (AnimSrc*)FindOptionalGafEntry(anims, 0, seqname);
    else
        *out = nullptr;
}

static inline void LoadFeatureSeqOnce(TdfRecord* rec, char* key, char* seqname, Gaf* anims, AnimSrc** out)
{
    int ok = rec->GetFieldString(seqname, key, 0x100, DAT_005119b8);
    if (ok) {
        *out = (AnimSrc*)FindOptionalGafEntry(anims, 0, seqname);
        if (*out)
            (*out)->kind = 0;
    } else {
        *out = nullptr;
    }
}

// FUNCTION: 0x4224b0
int __stdcall LoadFeatureType(char* name)
{
    Gaf* anims;
    int j;
    int ok;
    char seqname[0x100];
    char file[0x100];
    char path[0x100];

    TdfFile* entry = FindFeatureFile(name);
    if (entry == nullptr) {
        sprintf(path, "Record \"%s\" missing from feature files", name);
        FatalError(path);
    }
    g_game->features = (Feature*)GameReallocTagged(g_game->features, "FEATURES", (g_game->featureCount + 1) * 0x100);
    Feature* def = &g_game->features[g_game->featureCount];
    strncpy(def->name, name, 0x80);
    entry->current->GetFieldString(def->description, "Description", 0x14, DAT_005119b8);
    def->footprintX = entry->current->GetFieldInt("footprintx", 0);
    def->footprintZ = entry->current->GetFieldInt("footprintz", 0);
    def->height = entry->current->GetFieldInt("height", 0);
    ok = entry->current->GetFieldString(file, "object", 0x100, DAT_005119b8);
    if (ok) {
        def->noobject = 0;
        def->object = LoadObject3d(file);
    } else {
        def->noobject = 1;
        entry->current->GetFieldString(file, "filename", 0x100, DAT_005119b8);
        for (j = 0; j < g_game->featureCount; j++) {
            if (strncmp(g_game->features[j].filename, file, 0x10) == 0) {
                def->anims = nullptr;
                anims = g_game->features[j].anims;
                strcpy(def->filename, "REUSE");
                break;
            }
        }
        if (j == g_game->featureCount) {
            BuildDataPath(path, "anims", file, "GAF");
            def->anims = LoadGaf(path);
            strncpy(def->filename, file, 0x10);
            anims = def->anims;
        }
        LoadFeatureSeq(entry->current, "seqname", seqname, anims, &def->seq);
        LoadFeatureSeq(entry->current, "seqnameshad", seqname, anims, &def->seqshad);
        LoadFeatureSeqOnce(entry->current, "seqnameburn", seqname, anims, &def->seqburn);
        LoadFeatureSeqOnce(entry->current, "seqnameburnshad", seqname, anims, &def->seqburnshad);
        LoadFeatureSeqOnce(entry->current, "seqnamedie", seqname, anims, &def->seqdie);
        LoadFeatureSeqOnce(entry->current, "seqnamedieshad", seqname, anims, &def->seqdieshad);
        LoadFeatureSeqOnce(entry->current, "seqnamereclamate", seqname, anims, &def->seqreclamate);
        LoadFeatureSeqOnce(entry->current, "seqnamereclamateshad", seqname, anims, &def->seqreclamateshad);
    }
    def->spreadChance = entry->current->GetFieldInt("spreadchance", 0);
    def->seedChance = entry->current->GetFieldInt("reproduce", 0);
    def->seedSpread = entry->current->GetFieldInt("reproducearea", 0);
    def->metal = (unsigned short)entry->current->GetFieldInt("metal", 0);
    def->energy = (unsigned short)entry->current->GetFieldInt("energy", 0);
    def->damage = entry->current->GetFieldInt("damage", 0);
    def->animating = entry->current->GetFieldInt("animating", 0);
    def->animtrans = entry->current->GetFieldInt("animtrans", 0);
    def->shadtrans = entry->current->GetFieldInt("shadtrans", 0);
    def->flamable = entry->current->GetFieldInt("flamable", 0);
    def->geothermal = entry->current->GetFieldInt("geothermal", 0);
    def->blocking = entry->current->GetFieldInt("blocking", 0);
    def->reclaimable = entry->current->GetFieldInt("reclaimable", 0);
    def->autoreclaimable = entry->current->GetFieldInt("autoreclaimable", 1);
    def->indestructible = entry->current->GetFieldInt("indestructible", 0);
    def->nodisplayinfo = entry->current->GetFieldInt("nodisplayinfo", 0);
    def->nodrawundergray = entry->current->GetFieldInt("nodrawundergray", 0);
    if (_strcmpi(name, "DragonsTeeth") == 0)
        def->nodrawundergray = 1;
    if (_strcmpi(name, "DragonsTeeth_Core") == 0)
        def->nodrawundergray = 1;
    if (_strcmpi(name, "Fortification") == 0)
        def->nodrawundergray = 1;
    if (_strcmpi(name, "Fortification_Core") == 0)
        def->nodrawundergray = 1;
    def->burnTime = (short)(entry->current->GetFieldDouble("sparktime", 0.0) * 30.0);
    entry->current->GetFieldString(seqname, "burnweapon", 0x100, DAT_005119b8);
    def->burnweapon = (char*)g_seams.weapons->FindWeaponByName(seqname);
    def->anim.index = 0;
    def->anim.value = 0;
    def->anim.kind = 0;
    def->anim.src = nullptr;
    def->shadowAnim.index = 0;
    def->shadowAnim.value = 0;
    def->shadowAnim.kind = 0;
    def->shadowAnim.src = nullptr;
    if (def->animating) {
        if (def->seq)
            InitGafSequence(&def->anim, def->seq, 0);
        if (def->seqshad)
            InitGafSequence(&def->shadowAnim, def->seqshad, 0);
    }
    return g_game->featureCount++;
}

// FUNCTION: 0x422dd0
short __stdcall FindFeatureType(char* name)
{
    for (int i = 0; i < g_game->featureCount; i++) {
        if (_strcmpi(name, g_game->features[i].name) == 0) {
            return (short)i;
        }
    }
    return -1;
}

// FUNCTION: 0x422e40
unsigned short __stdcall FindOrLoadFeatureType(char* name)
{
    unsigned short i = (unsigned short)FindFeatureType(name);
    if (i == 0xffff)
        i = LoadFeatureType(name);
    return i;
}

// FUNCTION: 0x422ea0
void ResolveFeatureLinks()
{
    char buf[0x100];
    Feature** features;
    int i = 0;
    int* count = &g_game->featureCount;
    if (*count > 0) {
        features = &g_game->features;
        do {
            TdfRecord* parser = (TdfRecord*)FindFeatureFile((*features)[i].name)->current;
            if (parser->GetFieldString(buf, "featuredead", 0x100, DAT_005119b8))
                (*features)[i].dead = FindOrLoadFeatureType(buf);
            else
                (*features)[i].dead = 0xffff;
            if (parser->GetFieldString(buf, "featurereclamate", 0x100, DAT_005119b8))
                (*features)[i].reclamate = FindOrLoadFeatureType(buf);
            else
                (*features)[i].reclamate = 0xffff;
            if (parser->GetFieldString(buf, "featureburnt", 0x100, DAT_005119b8))
                (*features)[i].burnt = FindOrLoadFeatureType(buf);
            else
                (*features)[i].burnt = 0xffff;
            g_game->loadPercent = (unsigned char)((i + 1) * 100 / *count);
            i++;
        } while (i < *count);
    }
}

// FUNCTION: 0x423160
void PlaceMissionFeatures(void)
{
    for (int i = 0; i < g_game->net->featureCount; i++) {
        MissionFeature* p = &g_game->net->features[i];
        if (p->name[0] == 0)
            continue;
        unsigned short id = (unsigned short)FindFeatureType(p->name);
        if (id == 0xffff) {
            id = LoadFeatureType(p->name);
            if (id == 0xffff)
                continue;
        }
        Feature* f = &g_game->features[id];
        Cell* cell;
        if (f->flags16 & 1)
            cell = g_seams.los_radar->GetMapCell(p->x, p->y);
        else
            cell = g_seams.los_radar->GetMapCell(p->x - f->footprint.x / 2, p->y - f->footprint.y / 2);
        PlaceFeature(cell, id, 0, 0, 10);
    }
}

// FUNCTION: 0x4232a0
int AllocFeatureSpot()
{
    Pool* p = &g_game->pool;
    int i = p->freeHead;
    if (i == -1) {
        return 0x800;
    }
    MoveFeatureSpot(i, &p->usedHead);
    p->entries[i].used = 0;
    return i;
}

// FUNCTION: 0x4232f0
void __stdcall MoveFeatureSpot(int index, int* head)
{
    Pool* p = &g_game->pool;
    Spot* e = &p->entries[index];
    if (e->next == -1) {
        if (p->usedHead == index)
            p->usedHead = e->prev;
        else if (p->restHead == index)
            p->restHead = e->prev;
        else if (p->freeHead == index)
            p->freeHead = e->prev;
    } else {
        p->entries[e->next].prev = e->prev;
    }
    if (e->prev != -1)
        p->entries[e->prev].next = e->next;
    short old = *head;
    e->next = -1;
    e->prev = old;
    *head = index;
    if (e->prev != -1)
        p->entries[e->prev].next = index;
}

// FUNCTION: 0x4233a0
void __stdcall StartFeatureBurning(int x, int z, int flag)
{
    Cell* cell = g_seams.los_radar->GetMapCell(x, z);
    if (cell == 0)
        return;
    if (cell->feature >= 0xfffb)
        return;
    Feature* f = &g_game->features[cell->feature];
    if (f->seqburn == 0)
        return;
    if (cell->flags & 1)
        return;
    int i = AllocFeatureSpot();
    if (i >= 0x800)
        return;
    Spot* s = &g_game->pool.entries[i];
    s->feature = cell->feature;
    cell->spot = i;
    cell->flags |= 1;
    InitGafSequence(&s->anim, f->seqburn, 0);
    if (f->seqburnshad != 0) {
        InitGafSequence(&s->shadow, f->seqburnshad, 0);
        s->hasShadow = 1;
    } else {
        s->hasShadow = 0;
    }
    s->used = 1;
    s->x = x;
    s->z = z;
    s->burnTime = RandomInt(f->burnTime >> 1) + (f->burnTime >> 1);
    s->noSend = flag;
    Vec3 pos;
    pos.x = x << 20;
    pos.z = z << 20;
    PlaySoundAtByName("treeburn", &pos, 0);
    if (flag == 0) {
        FeatureDamagePacket packet;
        packet.type = 0xf;
        packet.sub = 0xfe;
        packet.x = x;
        packet.z = z;
        BroadcastPacket(GetLocalDpid(), &packet, 6);
    }
}

// FUNCTION: 0x423550
void __stdcall KillFeature(int x, int z, int flag)
{
    Cell* cell = g_seams.los_radar->GetMapCell(x, z);
    if (cell->feature == 0xfffe) {
        x -= cell->offsetX;
        z -= cell->offsetY;
        cell = g_seams.los_radar->GetMapCell(x, z);
    }
    if (cell->feature >= 0xfffb)
        return;
    Feature* f = &g_game->features[cell->feature];
    AnimPair pair;
    pair.anim = 0;
    pair.shadow = 0;
    if (f->flags & 1) {
        if (flag != 0)
            pair = f->death[1];
        else
            pair = f->death[0];
    }
    if (pair.anim != 0) {
        if (cell->flags & 1)
            return;
        int i = AllocFeatureSpot();
        if (i >= 0x800)
            return;
        Spot* s = &g_game->pool.entries[i];
        s->feature = cell->feature;
        s->bit4 = flag != 0;
        cell->spot = i;
        cell->flags |= 1;
        InitGafSequence(&s->anim, pair.anim, 0);
        if (pair.shadow != 0) {
            InitGafSequence(&s->shadow, pair.shadow, 0);
            s->hasShadow = 1;
        } else {
            s->hasShadow = 0;
        }
        s->used = 0;
        s->reclaimed = flag;
        s->x = x;
        s->z = z;
    } else {
        ReplaceFeatureWithDead(x, z, flag);
    }
}

// FUNCTION: 0x423710
void __stdcall ReplaceFeatureWithDead(int x, int y, int flag)
{
    Cell* cell = g_seams.los_radar->GetMapCell(x, y);
    if (cell == 0)
        return;
    if (cell->feature >= 0xfffb)
        return;
    Feature* f = &g_game->features[cell->feature];
    unsigned short v;
    if (flag != 0)
        v = f->reclamate;
    else
        v = f->dead;
    if (cell->flags & 1) {
        Spot* spot = (Spot *)g_game->spots.get() + cell->spot;
        if (spot->flags & 2)
            v = f->reclamate;
        RemoveFeature(cell, 0);
        PlaceFeature(cell, v, &spot->pos, &spot->rot, 10);
        return;
    }
    RemoveFeature(cell, 0);
    PlaceFeature(cell, v, 0, 0, 10);
}

static inline Feature* GetFeature(Cell* cell)
{
    if (cell == 0)
        return 0;
    if (cell->feature < 0xfffb) {
        if (cell->feature >= g_game->featureCount)
            return 0;
        return &g_game->features[cell->feature];
    }
    if (cell->feature != 0xfffe)
        return 0;
    if ((cell - (cell->offsetY * g_game->width + cell->offsetX))->feature >= 0xfffb)
        return 0;
    return &g_game->features[(cell - (cell->offsetY * g_game->width + cell->offsetX))->feature];
}

static inline void AddScaled(Unit* unit, float& res, double amount)
{
    if (unit->owner->active && unit->owner->type == 2) {
        switch (g_game->difficulty) {
        case 1:
            res += amount * 0.7;
            break;
        case 0:
            res += amount * 0.5;
            break;
        default:
            res += amount;
            break;
        }
    } else {
        res += amount;
    }
}
// FUNCTION: 0x4237d0
int __stdcall ReclaimFeature(Unit* unit, Vec3* pos)
{
    int x = pos->x / 0x100000;
    int z = pos->z / 0x100000;
    Cell* cell = g_seams.los_radar->GetMapCell(x, z);
    Feature* f = GetFeature(g_seams.los_radar->GetMapCell(x, z));
    if (f == 0)
        return 0;
    if ((cell->flags & 1) && (f->flags & 1))
        return 0;
    AddScaled(unit, unit->energy, f->energy);
    float metal = f->metal;
    if (unit->owner->active && unit->owner->type == 2) {
        switch (g_game->difficulty) {
        case 1:
            unit->metal += metal * 0.7;
            break;
        case 0:
            unit->metal += metal * 0.5;
            break;
        default:
            unit->metal += metal;
            break;
        }
    } else {
        unit->metal += metal;
    }
    KillFeature(x, z, 1);
    if (g_seams.map->GetGameType(g_game->net) == 3) {
        FeatureDamagePacket packet;
        packet.type = 0xf;
        packet.sub = 0xff;
        packet.x = x;
        packet.z = z;
        BroadcastPacket(((Player*)unit->player)->id, &packet, 6);
    }
    return 1;
}

// FUNCTION: 0x4239c0
void __stdcall SpreadFire(Feature* f, Point16* cell)
{
    for (int z = cell->y - 3; z <= cell->y + 3; z++) {
        for (int x = cell->x - 3; x <= cell->x + 3; x++) {
            if (x != cell->x || z != cell->y) {
                Cell* c = g_seams.los_radar->GetMapCell(x, z);
                if (c && c->feature < 0xfffb && !(c->flags & 1)) {
                    Feature* g = &g_game->features[c->feature];
                    if (g->flamable) {
                        if (RandomInt(100) < g->spreadChance)
                            StartFeatureBurning(x, z, 0);
                    }
                }
            }
        }
    }
    Coord px;
    Coord pz;
    px.value = cell->x << 16;
    pz.value = cell->y << 16;
    int lastX = cell->x;
    int lastZ = cell->y;
    for (int i = 5; i; i--) {
        px.value += (int)(((__int64)g_game->windX * 0x20000) >> 16);
        pz.value += (int)(((__int64)g_game->windZ * 0x20000) >> 16);
        int x = px.f.whole;
        int z = pz.f.whole;
        if (x != lastX || z != lastZ) {
            Cell* c = g_seams.los_radar->GetMapCell(x, z);
            if (c && c->feature < 0xfffb && !(c->flags & 1)) {
                Feature* g = &g_game->features[c->feature];
                if (g->flamable) {
                    if (RandomInt(100) < g->spreadChance)
                        StartFeatureBurning(x, z, 0);
                }
            }
            lastX = x;
            lastZ = z;
        }
    }
    if (f->burnweapon) {
        Vec3 pos = GetFootprintCentre(cell, f);
        g_seams.weapons->ApplyAreaDamageAt((WeaponDef*)f->burnweapon.get(), &pos);
    }
}

// FUNCTION: 0x423bf0
void __stdcall ReplaceFeatureWithBurnt(FeatureSite* obj)
{
    Cell* target = g_seams.los_radar->GetMapCell(obj->x, obj->y);
    if (target != 0) {
        unsigned short id = g_game->features[obj->entry].burnt;
        RemoveFeature(target, 0);
        if (id != 0xffff) {
            PlaceFeature(target, id, 0, 0, 10);
        }
    }
}

// FUNCTION: 0x423c50
Spot* __stdcall PlaceFeature(Cell* cell, unsigned short feature,
                                      Vec3* pos, Rot16* rot,
                                      unsigned char owner)
{
    Spot* spot = 0;
    if (feature == 0xffff)
        return 0;
    if (feature == 0xfffc) {
        cell->feature = feature;
        return 0;
    }
    Feature* f = &g_game->features[feature];
    int n = cell - g_game->cells;
    int w = g_game->width;
    Point16 at;
    at.x = n % w;
    at.y = n / w;
    if (at.x + f->footprint.x > w)
        return 0;
    if (at.y + f->footprint.y > g_game->height)
        return 0;
    for (int z = 0; z < f->footprint.y; z++) {
        Cell* c = &cell[z * g_game->width];
        for (int x = 0; x < f->footprint.x; x++, c++) {
            if (c->feature != 0xffff && !RemoveFeature(c, 0))
                return 0;
        }
    }
    if (!f->noobject) {
        int i = AllocFeatureSpot();
        if (i >= 0x800)
            return 0;
        spot = &g_game->pool.entries[i];
        spot->feature = feature;
        spot->damage = 0;
        spot->cell = at;
        if (pos)
            spot->pos = *pos;
        else
            spot->pos = GetFootprintCentre(&at, f);
        if (rot)
            spot->rot = *rot;
        else
            memset(&spot->rot, 0, sizeof(spot->rot));
        spot->state = (SpotState*)CreateObjectState((Object3do *)f->object);
        cell->feature = feature;
        cell->spot = i;
        cell->flag0 = 1;
    } else {
        cell->feature = feature;
        cell->spot = 0;
        cell->flag0 = 0;
    }
    cell->owner = owner;
    for (int z2 = 0; z2 < f->footprint.y; z2++) {
        Cell* c = &cell[z2 * g_game->width];
        for (int x = 0; x < f->footprint.x; x++, c++) {
            if (x != 0 || z2 != 0) {
                c->feature = 0xfffe;
                c->offsetX = x;
                c->offsetY = z2;
                c->flag0 = 0;
            }
        }
    }
    if (f->geothermal) {
        if (pos) {
            EmitTimedSubParticles(pos, 4);
        } else {
            Vec3 p = GetFootprintCentre(&at, f);
            EmitTimedSubParticles(&p, 4);
        }
    }
    int index = cell - g_game->cells;
    Point16 p2;
    p2.x = index % g_game->width;
    p2.y = index / g_game->width;
    g_seams.map->RefreshAllPassMaps(p2, f->footprint);
    return spot;
}

static inline SmokePos FootprintCentreSmoke(Point16* cell, Feature* def)
{
    Point16 f = def->footprint;
    Point16 c = *cell;
    SmokePos p;
    p.x.value = (f.x + c.x * 2) << 19;
    p.z.value = (f.y + c.y * 2) << 19;
    p.y.value = g_seams.units->GetGroundHeight((Vec3*)&p) << 16;
    return p;
}

static inline void BurnOut(Spot* spot)
{
    Cell* target = g_seams.los_radar->GetMapCell(spot->cell.x, spot->cell.y);
    if (target != 0) {
        unsigned short id = g_game->features[spot->feature].burnt;
        RemoveFeature(target, 0);
        if (id != 0xffff) {
            PlaceFeature(target, id, 0, 0, 10);
        }
    }
}

static inline int Rand(unsigned short n)
{
    return (int)((__int64)GameRand(RAND_STREAM_MAIN) * n / 0x8000);
}

static inline SmokePos SmokeAt(Spot* spot, Feature* f)
{
    SmokePos p = FootprintCentreSmoke(&spot->cell, f);
    GafFrame* frame = GetGafSequenceFrame(&spot->anim);
    unsigned short w = frame->width;
    p.x.f.whole += Rand(w >> 1) - (unsigned short)frame->xOffset + (unsigned short)(w >> 2);
    unsigned short h = frame->height;
    p.y.f.whole += ((unsigned short)frame->yOffset - Rand(h >> 1)) * 2 - ((unsigned short)(h >> 2) << 1);
    return p;
}
// FUNCTION: 0x424050
void __stdcall UpdateFeatures()
{
    Feature* types = g_game->features;
    for (int k = 0; k < g_game->featureCount; k++) {
        if (types[k].animating) {
            StepGafSequence(&types[k].anim);
            StepGafSequence(&types[k].shadowAnim);
        }
    }
    if (--g_game->scanIndex < 0) {
        g_game->scanIndex = g_game->width * g_game->height - 1;
    } else {
        Cell* c = &g_game->cells[g_game->scanIndex];
        if (c->feature < 0xfffb && !(c->flags & 1)) {
            Feature* f = &g_game->features[c->feature];
            if (RandomInt(100) < f->seedChance) {
                int x = g_game->scanIndex % g_game->width;
                int z = g_game->scanIndex / g_game->height;
                x += RandomInt(f->seedSpread) - f->seedSpread / 2;
                z += RandomInt(f->seedSpread) - f->seedSpread / 2;
                Cell* t = g_seams.los_radar->GetMapCell(x, z);
                if (t && c->unit == 0 && t->feature == 0xffff)
                    PlaceFeature(t, c->feature, 0, 0, 10);
            }
        }
    }
    int smoke = (unsigned int)g_game->ticks % 3 == 0;
    int i = g_game->pool.usedHead;
    while (i != -1) {
        Spot* spot = &g_game->pool.entries[i];
        Feature* f = &g_game->features[spot->feature];
        int next = spot->prev;
        if (!f->noobject) {
            if (Vec3NonZero(&spot->vel)) {
                spot->pos += spot->vel;
                int ground = g_seams.map->GetCellMeanHeight(&spot->pos) << 16;
                if (spot->pos.y <= ground) {
                    spot->pos.y = ground;
                    spot->vel = FeatureVec3(0, 0, 0);
                } else if (spot->pos.y < g_game->seaLevel << 16) {
                    spot->vel.x = 0;
                    spot->vel.y = -0x2ccc;
                    spot->vel.z = 0;
                } else {
                    spot->vel.y -= g_game->gravity;
                }
            } else {
                MoveFeatureSpot(i, &g_game->pool.restHead);
            }
        } else if (spot->flags & 1) {
            if (smoke) {
                SmokePos q = SmokeAt(spot, f);
                EmitWhiteSmoke(&q, 5);
            }
            StepGafSequence(&spot->anim);
            if (spot->flags & 4)
                StepGafSequence(&spot->shadow);
            if (spot->anim.src == 0) {
                BurnOut(spot);
            } else if (spot->timer > 0 && !(spot->flags & 8)) {
                if (--spot->timer == 0)
                    SpreadFire(f, &spot->cell);
            }
        } else {
            StepGafSequence(&spot->anim);
            if (spot->flags & 4)
                StepGafSequence(&spot->shadow);
            if (spot->anim.src == 0)
                ReplaceFeatureWithDead(spot->cell.x, spot->cell.y, 0);
        }
        i = next;
    }
}

// FUNCTION: 0x4244b0
void __stdcall DamageFeature(Cell* cell, int x, int z, WeaponDef* weapon)
{
    if (!(g_game->uiOptionFlags_37f2f_1 & 8))
        return;
    if (cell->feature >= 0xfffb)
        return;
    Feature* f = &g_game->features[cell->feature];
    if (f->indestructible)
        return;
    FeatureDamagePacket packet;
    int send;
    if (g_seams.map->GetGameType(g_game->net) == 3) {
        if (!(g_game->players[g_game->playerIndex].info->flags_97 & 1)) {
            packet.type = 0xf;
            packet.sub = weapon->index;
            packet.x = x;
            packet.z = z;
            SendPacketToPlayer(GetLocalDpid(), GetHostDpid(), (unsigned char *)&packet, 6);
            return;
        }
        send = 1;
        packet.sub = 0xfc;
    } else {
        send = 0;
    }
    if (f->flamable && weapon->fireStarter && !(cell->flags & 1)) {
        StartFeatureBurning(x, z, 0);
    } else if (cell->flags & 1) {
        if (!f->noobject) {
            Spot* s = &((Spot *)g_game->spots.get())[cell->spot];
            if (s->x != x)
                return;
            if (s->z != z)
                return;
            s->damage += weapon->damage;
            if (s->damage >= f->damage) {
                KillFeature(s->x, s->z, 0);
                packet.sub = 0xfd;
            }
        }
    } else {
        int d = weapon->damage + cell->spot;
        if (d >= f->damage) {
            KillFeature(x, z, 0);
            packet.sub = 0xfd;
        } else {
            cell->spot = d;
        }
    }
    if (send && packet.sub > 0xfc) {
        packet.type = 0xf;
        packet.x = x;
        packet.z = z;
        BroadcastPacket(GetLocalDpid(), &packet, 6);
    }
}

static inline void ClearCell(Cell* c)
{
    c->feature = 0xffff;
    c->flags &= 0xfe;
}
// FUNCTION: 0x4246b0
int __stdcall RemoveFeature(Cell* cell, int flag)
{
    if (cell->feature == 0xfffe)
        cell -= cell->offsetY * g_game->width + cell->offsetX;
    if (cell->feature >= 0xfffb)
        return 0;
    Feature* f = &g_game->features[cell->feature];
    if (flag == 0 && (f->flags16 & 0x200))
        return 0;
    if (cell->flags & 1) {
        Spot* spot = &((Spot *)g_game->spots.get())[cell->spot];
        if (!(f->flags16 & 1)) {
            FreeObjectState(spot->state);
            spot->state = 0;
        }
        MoveFeatureSpot(cell->spot, &g_game->pool.freeHead);
    }
    ClearCell(cell);
    for (int y = 0; y < f->footprint.y; y++) {
        Cell* row = &cell[y * g_game->width];
        for (int x = 0; x < f->footprint.x; x++) {
            if (row[x].feature == 0xfffe)
                ClearCell(&row[x]);
        }
    }
    int index = cell - g_game->cells;
    Point16 p;
    p.x = index % g_game->width;
    p.y = index / g_game->width;
    g_seams.map->RefreshAllPassMaps(p, f->footprint);
    return 1;
}

// FUNCTION: 0x424840
void RemoveAllFeatures(void)
{
    int n = g_game->width * g_game->height;
    for (int i = 0; i < n; i++) {
        Cell* c = &g_game->cells[i];
        if (c->feature < 0xfffb || c->feature == 0xfffe) {
            RemoveFeature(c, 1);
        }
    }
}

// FUNCTION: 0x424890
void __stdcall SaveFeatures(HapiBank* file)
{
    file->OpenAccount("Features");
    std::vector<FeatureName> names(g_game->featureCount);
    FeatureName* dst = names.begin();
    Feature* src = g_game->features;
    for (int i = 0; i < g_game->featureCount; i++, dst++, src++)
        strncpy(dst->name, src->name, 0x80);
    file->OpenNamedBox("Feature Type Names");
    file->WriteBox(names.begin(), g_game->featureCount * sizeof(FeatureName));

    Cell* c = g_game->cells;
    Cell* end = g_game->cells + g_game->width * g_game->height;
    int normalCount = 0;
    int modelCount = 0;
    int animCount = 0;
    int x = 0;
    int y = 0;
    for (; c < end; c++) {
        if (c->feature < 0xfffb) {
            Feature* f = &g_game->features[c->feature];
            if (!(f->flags & 1)) {
                Model rec;
                Spot* s = &((Spot *)g_game->spots.get())[c->spot];
                rec.x = x;
                rec.y = y;
                rec.feature = c->feature;
                rec.damage = s->damage;
                rec.pos = s->pos2;
                rec.rot = s->rot;
                file->OpenNamedBox("3D Features");
                file->SeekBox(file->GetBoxSize());
                file->WriteBox(&rec, 0x1a);
                modelCount++;
            } else if (c->flags & 1) {
                AnimRecord rec;
                Spot* s = &((Spot *)g_game->spots.get())[c->spot];
                rec.x = x;
                rec.y = y;
                rec.feature = c->feature;
                rec.damage = s->damage;
                rec.frame = s->frame;
                rec.animHi = s->animHi;
                if (s->animIndex == f->animIds[0])
                    rec.anim = 0;
                else if (s->animIndex == f->animIds[2])
                    rec.anim = 1;
                else if (s->animIndex == f->animIds[4])
                    rec.anim = 2;
                else
                    goto next;
                file->OpenNamedBox("Animating Features");
                file->SeekBox(file->GetBoxSize());
                file->WriteBox(&rec, 10);
                animCount++;
            } else {
                Normal rec;
                rec.x = x;
                rec.y = y;
                rec.feature = c->feature;
                rec.spot = c->spot;
                file->OpenNamedBox("Normal Features");
                file->SeekBox(file->GetBoxSize());
                file->WriteBox(&rec, 8);
                normalCount++;
            }
        }
    next:
        x++;
        if (x >= g_game->width) {
            x = 0;
            y++;
        }
    }
    file->SetIntegerItem("Number of Normal Features", normalCount);
    file->SetIntegerItem("Number of 3D Features", modelCount);
    file->SetIntegerItem("Number of Animating Features", animCount);
}

// FUNCTION: 0x424c00
void __stdcall LoadFeatures(HapiBank* file)
{
    int j;
    file->OpenAccount("Features");
    std::vector<unsigned short> remap;
    LoadFeatureFileList();
    if (file->OpenNamedBox("Feature Type Names")) {
        int count = file->GetBoxSize() / sizeof(FeatureName);
        remap.resize(count);
        std::vector<FeatureName> names(count);
        file->ReadBox(names.begin(), count * (int)sizeof(FeatureName));
        for (int i = 0; i < count; i++) {
            if (i < g_game->featureCount && _strcmpi(names[i].name, g_game->features[i].name) == 0) {
                remap[i] = i;
                continue;
            }
            for (j = 0; j < g_game->featureCount; j++) {
                if (_strcmpi(names[i].name, g_game->features[j].name) == 0) {
                    remap[i] = j;
                    goto next;
                }
            }
            remap[i] = FindOrLoadFeatureType(names[i].name);
        next:;
        }
    } else {
        remap.resize(g_game->featureCount);
        int i = 0;
        for (; i < g_game->featureCount; i++)
            remap[i] = i;
    }
    ResolveFeatureLinks();
    FreeFeatureFileList();

    int k, n;
    n = file->GetIntegerItem("Number of Normal Features", 0);
    file->OpenNamedBox("Normal Features");
    for (k = 0; k < n; k++) {
        Normal rec;
        file->SeekBox(k * sizeof(Normal));
        int got = file->ReadBox(&rec, sizeof(Normal));
        if (got >= sizeof(Normal)) {
            Cell* c = g_seams.los_radar->GetMapCell((unsigned short)rec.x, (unsigned short)rec.y);
            unsigned short type = remap[rec.feature];
            PlaceFeature(c, type, nullptr, nullptr, 10);
            c->spot = rec.spot;
        }
    }

    n = file->GetIntegerItem("Number of Animating Features", 0);
    file->OpenNamedBox("Animating Features");
    for (k = 0; k < n; k++) {
        AnimRecord rec;
        file->SeekBox(k * sizeof(AnimRecord));
        int got = file->ReadBox(&rec, sizeof(AnimRecord));
        if (got >= sizeof(AnimRecord)) {
            Cell* c = g_seams.los_radar->GetMapCell((unsigned short)rec.x, (unsigned short)rec.y);
            unsigned short type = remap[rec.feature];
            PlaceFeature(c, type, nullptr, nullptr, 10);
            switch (rec.anim) {
            case 0:
                StartFeatureBurning((unsigned short)rec.x, (unsigned short)rec.y, 0);
                break;
            case 1:
                KillFeature((unsigned short)rec.x, (unsigned short)rec.y, 0);
                break;
            case 2:
                KillFeature((unsigned short)rec.x, (unsigned short)rec.y, 1);
                break;
            }
            Spot* s = &((Spot*)g_game->spots.get())[c->spot];

            s->damage = rec.damage;
            *(unsigned short*)&s->frame = rec.frame;
            s->animBits = (unsigned char)(rec.animHi << 4);
        }
    }

    n = file->GetIntegerItem("Number of 3D Features", 0);
    file->OpenNamedBox("3D Features");
    for (k = 0; k < n; k++) {
        Model rec;
        file->SeekBox(k * sizeof(Model));
        int got = file->ReadBox(&rec, sizeof(Model));
        if (got >= sizeof(Model)) {
            Cell* c = g_seams.los_radar->GetMapCell((unsigned short)rec.x, (unsigned short)rec.y);
            unsigned short type = remap[rec.feature];
            PlaceFeature(c, type, &rec.pos, &rec.rot, 10);
            ((Spot*)g_game->spots.get())[c->spot].damage = rec.damage;
        }
    }
}

typedef std::vector<unsigned short> UShortVectorSize;
typedef UShortVectorSize::size_type (UShortVectorSize::*UShortVectorSizeFn)() const;

// FUNCTION: 0x4251f0 ?size@?$vector@GV?$allocator@G@std@@@std@@QBEIXZ
UShortVectorSizeFn g_size_004251f0 = &UShortVectorSize::size;

typedef std::vector<unsigned short> UShortVectorInsert;
typedef void (UShortVectorInsert::*UShortVectorInsertFn)(
    UShortVectorInsert::iterator, UShortVectorInsert::size_type, const unsigned short&);

void __stdcall ReserveUShortVector(UShortVectorInsert* v, int extra)
{
    v->reserve(extra + v->size());
}

// FUNCTION: 0x425210 ?insert@?$vector@GV?$allocator@G@std@@@std@@QAEXPAGIABG@Z
UShortVectorInsertFn g_insert_00425210 = &UShortVectorInsert::insert;

typedef std::vector<unsigned short> UShortVectorErase;
typedef UShortVectorErase::iterator (UShortVectorErase::*UShortVectorEraseFn)(
    UShortVectorErase::iterator, UShortVectorErase::iterator);

// FUNCTION: 0x425430 ?erase@?$vector@GV?$allocator@G@std@@@std@@QAEPAGPAG0@Z
UShortVectorEraseFn g_erase_00425430 = &UShortVectorErase::erase;

typedef std::vector<TdfFile*> TdfFileVectorInsert;
typedef void (TdfFileVectorInsert::*TdfFileVectorInsertFn)(
    TdfFileVectorInsert::iterator, TdfFileVectorInsert::size_type, TdfFile* const&);

void __stdcall AssignTdfFileVector(TdfFileVectorInsert* a, const TdfFileVectorInsert* b)
{
    *a = *b;
}

// FUNCTION: 0x425480 ?insert@?$vector@PAVTdfFile@@V?$allocator@PAVTdfFile@@@std@@@std@@QAEXPAPAVTdfFile@@IABQAV3@@Z
TdfFileVectorInsertFn g_insert_00425480 = &TdfFileVectorInsert::insert;

namespace std {
// FUNCTION: 0x4256a0 ?copy@std@@YGPAGPAG00@Z
template <>
unsigned short* __stdcall copy(unsigned short* first, unsigned short* last, unsigned short* dest)
{
    for (; first != last; ++dest, ++first)
        *dest = *first;
    return dest;
}
}
