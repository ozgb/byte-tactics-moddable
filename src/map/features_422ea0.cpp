#include <string.h>

#include "../util/tdf.h"


struct List_00422ea0 {
    int unknown_0;
    TdfFile** first;
    TdfFile** last;
};

struct FeatureDef_00422ea0 {
    char name[0xf4];
    unsigned short dead;
    unsigned short burnt;
    unsigned short reclamate;
    char unknown_fa[0x100 - 0xfa];
};

#pragma pack(push, 1)
#include "../game/game.h"
#include "../state/sim_globals.h"
#pragma pack(pop)

extern Game* g_game;
extern List_00422ea0* s_featureTdfParsers;

unsigned short __stdcall LoadFeatureType(char* name);

static inline TdfFile* FindEntry(char* name)
{
    for (TdfFile** p = s_featureTdfParsers->first; p < s_featureTdfParsers->last; p++) {
        (*p)->ResetCurrentRecord();
        if ((*p)->SelectRecord(name))
            return *p;
    }
    return 0;
}

static inline unsigned short FindName(char* name)
{
    for (int i = 0; i < g_game->featureCount; i++) {
        if (_strcmpi(name, ((FeatureDef_00422ea0 *)g_game->features)[i].name) == 0) {
            return (unsigned short)i;
        }
    }
    return 0xffff;
}

static inline unsigned short FeatureIndex(char* name)
{
    unsigned short i = FindName(name);
    if (i != 0xffff)
        return i;
    return LoadFeatureType(name);
}

// FUNCTION: 0x422ea0
void ResolveFeatureLinks()
{
    char buf[0x100];
    FeatureDef_00422ea0** features;
    int i = 0;
    int* count = &g_game->featureCount;
    if (*count > 0) {
        features = &(*(FeatureDef_00422ea0 **)&g_game->features);
        do {
            TdfRecord* parser = (TdfRecord*)FindEntry((*features)[i].name)->current;
            if (parser->GetFieldString(buf, "featuredead", 0x100, DAT_005119b8))
                (*features)[i].dead = FeatureIndex(buf);
            else
                (*features)[i].dead = 0xffff;
            if (parser->GetFieldString(buf, "featurereclamate", 0x100, DAT_005119b8))
                (*features)[i].reclamate = FeatureIndex(buf);
            else
                (*features)[i].reclamate = 0xffff;
            if (parser->GetFieldString(buf, "featureburnt", 0x100, DAT_005119b8))
                (*features)[i].burnt = FeatureIndex(buf);
            else
                (*features)[i].burnt = 0xffff;
            g_game->loadPercent = (unsigned char)((i + 1) * 100 / *count);
            i++;
        } while (i < *count);
    }
}
