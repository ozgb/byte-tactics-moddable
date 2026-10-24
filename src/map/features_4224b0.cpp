#include "win_types.h"
#include "../../interfaces/ref.h"
#include <stdio.h>
#include <string.h>

#include "../util/tdf.h"

struct List_004224b0 {
    int unknown_0;
    TdfFile** first;
    TdfFile** last;
};

struct Seq_004224b0 {
    unsigned short count;
    unsigned char kind;
};

struct Gaf_004224b0;

struct Ref_004224b0 {
    unsigned short index;
    unsigned short value;
    unsigned char kind;
    char unknown_5[3];
    Seq_004224b0* src;
};

struct FeatureDef_004224b0 {
    char name[0x80];
    char description[0x14];
    short footprintx;
    short footprintz;
    union {
        void* object;
        char filename[0x10];
    };
    Gaf_004224b0* anims;
    Seq_004224b0* seq;
    Seq_004224b0* seqshad;
    Seq_004224b0* seqburn;
    Seq_004224b0* seqburnshad;
    Seq_004224b0* seqdie;
    Seq_004224b0* seqdieshad;
    Seq_004224b0* seqreclamate;
    Seq_004224b0* seqreclamateshad;
    Ref_004224b0 ref;
    Ref_004224b0 refshad;
    Ref<char> burnweapon;
    short sparktime;
    short damage;
    float energy;
    float metal;
    unsigned short dead;
    unsigned short burnt;
    unsigned short reclamate;
    char height;
    char spreadchance;
    char reproduce;
    char reproducearea;
    unsigned short noobject : 1;
    unsigned short animating : 1;
    unsigned short animtrans : 1;
    unsigned short shadtrans : 1;
    unsigned short flamable : 1;
    unsigned short geothermal : 1;
    unsigned short blocking : 1;
    unsigned short reclaimable : 1;
    unsigned short autoreclaimable : 1;
    unsigned short indestructible : 1;
    unsigned short nodisplayinfo : 1;
    unsigned short nodrawundergray : 1;
};

#pragma pack(push, 1)
#include "../game/game.h"
#include "../state/sim_globals.h"
#pragma pack(pop)

#include "../../link/globals.h"
extern List_004224b0* s_featureTdfParsers;

void __stdcall BuildDataPath(char* out, const char* dir, const char* name, const char* ext);
void __stdcall FatalError(char* path);
void* __cdecl GameReallocTagged(void* p, const char* name, unsigned int size);
#include "../game/data_files.h"
Gaf_004224b0* __stdcall LoadGaf(char* path);
Seq_004224b0* __stdcall FindGafEntry(Gaf_004224b0* gaf, const char* name);
void __stdcall InitGafSequence(Ref_004224b0* ref, Seq_004224b0* src, int index);

static inline TdfFile* FindEntry(char* name)
{
    for (TdfFile** p = s_featureTdfParsers->first; p < s_featureTdfParsers->last; p++) {
        (*p)->ResetCurrentRecord();
        if ((*p)->SelectRecord(name))
            return *p;
    }
    return 0;
}

static inline Seq_004224b0* SeqByName(Gaf_004224b0* gaf, int unused, char* name)
{
    if (strlen(name) == 0) {
        return 0;
    }
    return FindGafEntry(gaf, name);
}

static inline void LoadFeatureSeq(TdfRecord* rec, char* key, char* seqname, Gaf_004224b0* anims, Seq_004224b0** out)
{
    int ok = rec->GetFieldString(seqname, key, 0x100, DAT_005119b8);
    if (ok)
        *out = SeqByName(anims, 0, seqname);
    else
        *out = 0;
}

static inline void LoadFeatureSeqOnce(TdfRecord* rec, char* key, char* seqname, Gaf_004224b0* anims, Seq_004224b0** out)
{
    int ok = rec->GetFieldString(seqname, key, 0x100, DAT_005119b8);
    if (ok) {
        *out = SeqByName(anims, 0, seqname);
        if (*out)
            (*out)->kind = 0;
    } else {
        *out = 0;
    }
}

#include "../../interfaces/seams.h"

// FUNCTION: 0x4224b0
int __stdcall LoadFeatureType(char* name)
{
    Gaf_004224b0* anims;
    int j;
    int ok;
    char seqname[0x100];
    char file[0x100];
    char path[0x100];

    TdfFile* entry = FindEntry(name);
    if (entry == 0) {
        sprintf(path, "Record \"%s\" missing from feature files", name);
        FatalError(path);
    }
    (*(FeatureDef_004224b0 **)&g_game->features) = (FeatureDef_004224b0*)GameReallocTagged((FeatureDef_004224b0 *)g_game->features, "FEATURES",
                                                          (g_game->featureCount + 1) * 0x100);
    FeatureDef_004224b0* def = &((FeatureDef_004224b0 *)g_game->features)[g_game->featureCount];
    strncpy(def->name, name, 0x80);
    entry->current->GetFieldString(def->description, "Description", 0x14, DAT_005119b8);
    def->footprintx = entry->current->GetFieldInt("footprintx", 0);
    def->footprintz = entry->current->GetFieldInt("footprintz", 0);
    def->height = entry->current->GetFieldInt("height", 0);
    ok = entry->current->GetFieldString(file, "object", 0x100, DAT_005119b8);
    if (ok) {
        def->noobject = 0;
        def->object = LoadObject3d(file);
    } else {
        def->noobject = 1;
        entry->current->GetFieldString(file, "filename", 0x100, DAT_005119b8);
        for (j = 0; j < g_game->featureCount; j++) {
            if (strncmp(((FeatureDef_004224b0 *)g_game->features)[j].filename, file, 0x10) == 0) {
                def->anims = 0;
                anims = ((FeatureDef_004224b0 *)g_game->features)[j].anims;
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
    def->spreadchance = entry->current->GetFieldInt("spreadchance", 0);
    def->reproduce = entry->current->GetFieldInt("reproduce", 0);
    def->reproducearea = entry->current->GetFieldInt("reproducearea", 0);
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
    def->sparktime = (short)(entry->current->GetFieldDouble("sparktime", 0.0) * 30.0);
    entry->current->GetFieldString(seqname, "burnweapon", 0x100, DAT_005119b8);
    def->burnweapon = (char*)g_seams.weapons->FindWeaponByName(seqname);
    def->ref.index = 0;
    def->ref.value = 0;
    def->ref.kind = 0;
    def->ref.src = 0;
    def->refshad.index = 0;
    def->refshad.value = 0;
    def->refshad.kind = 0;
    def->refshad.src = 0;
    if (def->animating) {
        if (def->seq)
            InitGafSequence(&def->ref, def->seq, 0);
        if (def->seqshad)
            InitGafSequence(&def->refshad, def->seqshad, 0);
    }
    return g_game->featureCount++;
}
