#include <string.h>
#include "../state/block.h"

struct NameEntry_00421f20 {
    char unknown_0[4];
    char name[0x80];
};

struct List_00421f20 {
    char unknown_0[0x1c];
    int count;
    NameEntry_00421f20* entries;
};

#pragma pack(push, 1)
#include "../game/game.h"
#pragma pack(pop)

#include "../../link/globals.h"

unsigned short __stdcall LoadFeatureType(char* name);

// FUNCTION: 0x421f20
void __stdcall InitFeatureAnimPool(List_00421f20* list)
{
    FeatureAnimPool* m = &g_game->anim;

    m->anim = (AnimEntry_00421f20*)SimAllocTagged("FEATURE ANIM DATA", 0x18000);
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

    m->featureUnit = (FeatureUnit_00421f20*)SimAllocTagged("Feature Unit", 0x118);
    memset(m->featureUnit, 0, 0x118);
    m->featureUnit->flags_110 |= 0x200;
    m->featureUnit->flags_110 |= 0x20000000;
    m->featureUnit->unitDefIndex = 0;
    m->featureUnit->flags_114 |= 1;
    m->featureUnit->def = g_game->defs;
    m->names = 0;
    m->nameCount = 0;

    NameEntry_00421f20*& entries = list->entries;

    for (int j = 0; j < list->count; j++) {
        LoadFeatureType(entries[j].name);
    }
}
