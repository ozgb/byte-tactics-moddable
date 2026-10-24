#include "win_types.h"
#include "ta_types.h"
#include <string.h>

struct Spot_00424c00 {
    char unknown_0[4];
    unsigned short frame;
    char unknown_6[0x26 - 6];
    unsigned short damage;
    char unknown_28[0x2e - 0x28];
    unsigned char animBits;
    char unknown_2f;
};

struct Cell_00424c00 {
    char unknown_0[8];
    unsigned short feature;
    unsigned short spot;
    unsigned char flags;
};

struct Vec3_00424c00 {
    int x, y, z;
};

#pragma pack(push, 1)
struct Anim_00424c00 {
    unsigned short x;
    unsigned short y;
    unsigned short feature;
    unsigned short damage;
    unsigned char frame;
    union {
        unsigned char bits;
        struct {
            unsigned char anim : 4;
            unsigned char animHi : 4;
        };
    };
};

struct Model3D_00424c00 {
    unsigned short x;
    unsigned short y;
    unsigned short feature;
    unsigned short damage;
    Vec3_00424c00 pos;
    Rot16 rot;
};
#pragma pack(pop)

#include "../../link/globals.h"
typedef std::vector<TdfFile*> FeatureList;
#ifdef __clang__
static FeatureList* volatile s_featureTdfParsers;
#else
static FeatureList* s_featureTdfParsers;
#endif

#include "features.h"
unsigned short __stdcall LoadFeatureType(char* name);
#include "features_422ea0.h"
void* __stdcall PlaceFeature(Cell_00424c00* cell, unsigned short feature, void* pos, void* rot, unsigned char owner);

static inline unsigned short FindName(char* name)
{
    for (int i = 0; i < g_game->featureCount; i++) {
        if (_strcmpi(name, g_game->features[i].name) == 0) return (unsigned short)i;
    }
    return 0xffff;
}

static inline unsigned short FeatureIndex(char* name)
{
    unsigned short i = FindName(name);
    return i != 0xffff ? i : LoadFeatureType(name);
}

static inline void FreeFeatureList()
{
    for (TdfFile** p = s_featureTdfParsers->begin(); p < s_featureTdfParsers->end(); p++)
        delete *p;
    delete s_featureTdfParsers;
    s_featureTdfParsers = 0;
}

#include "../../interfaces/seams.h"
#include "../files/files_api.h"

// FUNCTION: 0x424c00
void __stdcall LoadFeatures(HapiBank* file)
{
    int j;
    file->OpenAccount("Features");
    std::vector<unsigned short> remap;
    LoadFeatureFileList();
    if (file->OpenNamedBox("Feature Type Names")) {
        int count = ((Class_004b4bf0*)file)->GetBoxSize() / sizeof(FeatureName_00424c00);
        remap.resize(count);
        std::vector<FeatureName_00424c00> names(count);
        file->ReadBox(names.begin(), count * (int)sizeof(FeatureName_00424c00));
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
            remap[i] = FeatureIndex(names[i].name);
        next:;
        }
    } else {
        remap.resize(g_game->featureCount);
        int i = 0;
        for (; i < g_game->featureCount; i++) remap[i] = i;
    }
    ResolveFeatureLinks();
    FreeFeatureList();

    int k, n;
    n = file->GetIntegerItem("Number of Normal Features", 0);
    file->OpenNamedBox("Normal Features");
    for (k = 0; k < n; k++) {
        Normal_00424890 rec;
        ((Class_004b4c10*)file)->SeekBox(k * sizeof(Normal_00424890));
        int got = file->ReadBox(&rec, sizeof(Normal_00424890));
        if (got >= sizeof(Normal_00424890)) {
            Cell_00424c00* c = (Cell_00424c00*)g_seams.los_radar->GetMapCell(rec.x, rec.y);
            unsigned short type = remap[rec.feature];
            PlaceFeature(c, type, 0, 0, 10);
            c->spot = rec.spot;
        }
    }

    n = file->GetIntegerItem("Number of Animating Features", 0);
    file->OpenNamedBox("Animating Features");
    for (k = 0; k < n; k++) {
        Anim_00424c00 rec;
        ((Class_004b4c10*)file)->SeekBox(k * sizeof(Anim_00424c00));
        int got = file->ReadBox(&rec, sizeof(Anim_00424c00));
        if (got >= sizeof(Anim_00424c00)) {
            Cell_00424c00* c = (Cell_00424c00*)g_seams.los_radar->GetMapCell(rec.x, rec.y);
            unsigned short type = remap[rec.feature];
            PlaceFeature(c, type, 0, 0, 10);
            switch (rec.anim) {
            case 0:
                StartFeatureBurning(rec.x, rec.y, 0);
                break;
            case 1:
                KillFeature(rec.x, rec.y, 0);
                break;
            case 2:
                KillFeature(rec.x, rec.y, 1);
                break;
            }
            Spot_00424c00* s = (Spot_00424c00*)&g_game->spots[c->spot];

            s->damage = rec.damage;
            s->frame = rec.frame;
            s->animBits = rec.bits & 0xf0;
        }
    }

    n = file->GetIntegerItem("Number of 3D Features", 0);
    file->OpenNamedBox("3D Features");
    for (k = 0; k < n; k++) {
        Model3D_00424c00 rec;
        ((Class_004b4c10*)file)->SeekBox(k * sizeof(Model3D_00424c00));
        int got = file->ReadBox(&rec, sizeof(Model3D_00424c00));
        if (got >= sizeof(Model3D_00424c00)) {
            Cell_00424c00* c = (Cell_00424c00*)g_seams.los_radar->GetMapCell(rec.x, rec.y);
            unsigned short type = remap[rec.feature];
            PlaceFeature(c, type, &rec.pos, &rec.rot, 10);
            g_game->spots[c->spot].damage = rec.damage;
        }
    }
}
