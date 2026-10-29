#include <stdio.h>
#include <string.h>

class TdfFile;

#include "../util/tdf.h"
#include "../graphics/rect.h"

#pragma pack(push, 1)
#include "game.h"
#include "../state/sim_globals.h"

#pragma pack(pop)

#include "../../link/globals.h"

#include "data_files.h"
void* __stdcall LoadGaf(char* path);
void* __stdcall FindGafEntry(void* gaf, const char* name);
#include "../graphics/display.h"
#include "../graphics/object_pool.h"
#include "side_data.h"
#include "../files/files_api.h"

// FUNCTION: 0x4318c0
void LoadLogos()
{
    char path[256];
    BuildDataPath(path, "textures", "logos", "GAF");
    g_game->logos = LoadGaf(path);
    g_game->logos32 = FindGafEntry(g_game->logos, "32xlogos");
}

// FUNCTION: 0x431920
void FreeLogos(void)
{
    int val = ((int)g_game->logos);
    GameFreeThunk((int*)val);
    g_game->logos = (void*)(0);
}

// FUNCTION: 0x431950
void __stdcall ReadSideRect(TdfFile* obj, int* out, char* name, char* side)
{
    int saved = obj->GetCurrentRecord();
    if (!obj->SelectRecord(name)) {
        char buf[256];
        sprintf(buf, "No [%s] in GAMEDATA/SIDEDATA.TDF for side:%s", name, side);
        FatalError(buf);
        obj->SetCurrentRecord(saved);
        return;
    }
    out[0] = obj->current->GetFieldInt("x1", 0);
    out[1] = obj->current->GetFieldInt("y1", 0);
    out[2] = obj->current->GetFieldInt("x2", 0);
    out[3] = obj->current->GetFieldInt("y2", 0);
    obj->SetCurrentRecord(saved);
}

// FUNCTION: 0x431a20
void FreeSideFonts()
{
    for (int i = 0; i < 5; i++) {
        Font*& buffer = g_game->sideDefs[i].font;
        if (buffer) {
            GameFreeThunk(buffer);
            buffer = 0;
        }
    }
}

static inline void LoadSideRect(TdfFile* parser, int* out, char* name, char* side, char* msg)
{
    int saved = parser->GetCurrentRecord();
    if (!parser->SelectRecord(name)) {
        sprintf(msg, "No [%s] in GAMEDATA/SIDEDATA.TDF for side:%s", name, side);
        FatalError(msg);
    } else {
        out[0] = parser->current->GetFieldInt("x1", 0);
        out[1] = parser->current->GetFieldInt("y1", 0);
        out[2] = parser->current->GetFieldInt("x2", 0);
        out[3] = parser->current->GetFieldInt("y2", 0);
    }
    parser->SetCurrentRecord(saved);
}

// FUNCTION: 0x431a60
void __stdcall LoadSideData(void)
{
    TdfFile parser;
    int side;
    char name[0x100];
    char fontPath[0x100];
    char msgEnergynum[0x100];
    char msgTotalunits[0x100];
    char msgTotaltime[0x100];
    char msgLogo[0x100];
    char msgEnergy0[0x100];
    char msgMetalnum[0x100];
    char msgMetal0[0x100];
    char msgMetalbar[0x100];
    char msgEnergymax[0x100];
    char msgEnergybar[0x100];
    char msgMetalmax[0x100];
    SideDef* s;

    BuildDataPath(name, "gamedata", "sidedata", "TDF");
    parser.LoadFile(name);
    s = g_game->sideDefs;
    side = 0;
    while (1) {
        s->sideNumber = side;
        sprintf(name, "SIDE%d", side);
        parser.ResetCurrentRecord();
        if (!parser.SelectRecord(name))
            break;
        if (parser.current->GetFieldString(name, "name", 0x1e, DAT_005119b8))
            strcpy(s->name, name);
        if (parser.current->GetFieldString(name, "nameprefix", 4, DAT_005119b8))
            strcpy(s->nameprefix, name);
        if (parser.current->GetFieldString(name, "commander", 0x20, DAT_005119b8))
            strcpy(s->commander, name);
        if (parser.current->GetFieldString(name, "font", 0x100, DAT_005119b8)) {
            Font* font;
            BuildDataPath(fontPath, "fonts", name, "FNT");
            font = (Font*)HAPI_LoadFile(fontPath, 0);
            if (font == 0)
                FatalError(fontPath);
            s->font = font;
        }
        s->energyColor = parser.current->GetFieldInt("energycolor", 0);
        s->metalColor = parser.current->GetFieldInt("metalcolor", 0);

        LoadSideRect(&parser, &s->logo.left, "LOGO", s->name, msgLogo);

        LoadSideRect(&parser, &s->energyBar.left, "ENERGYBAR", s->name, msgEnergybar);

        LoadSideRect(&parser, &s->energyNum.left, "ENERGYNUM", s->name, msgEnergynum);

        LoadSideRect(&parser, &s->metalBar.left, "METALBAR", s->name, msgMetalbar);

        LoadSideRect(&parser, &s->metalNum.left, "METALNUM", s->name, msgMetalnum);

        LoadSideRect(&parser, &s->totalUnits.left, "TOTALUNITS", s->name, msgTotalunits);

        LoadSideRect(&parser, &s->totalTime.left, "TOTALTIME", s->name, msgTotaltime);

        LoadSideRect(&parser, &s->energy0.left, "ENERGY0", s->name, msgEnergy0);

        LoadSideRect(&parser, &s->metal0.left, "METAL0", s->name, msgMetal0);

        LoadSideRect(&parser, &s->energyMax.left, "ENERGYMAX", s->name, msgEnergymax);

        LoadSideRect(&parser, &s->metalMax.left, "METALMAX", s->name, msgMetalmax);

        ReadSideRect(&parser, &s->energyProduced.left, "ENERGYPRODUCED", s->name);
        ReadSideRect(&parser, &s->energyConsumed.left, "ENERGYCONSUMED", s->name);
        ReadSideRect(&parser, &s->metalProduced.left, "METALPRODUCED", s->name);
        ReadSideRect(&parser, &s->metalConsumed.left, "METALCONSUMED", s->name);
        ReadSideRect(&parser, &s->logo2.left, "LOGO2", s->name);
        ReadSideRect(&parser, &s->unitName.left, "UNITNAME", s->name);
        ReadSideRect(&parser, &s->damageBar.left, "DAMAGEBAR", s->name);
        ReadSideRect(&parser, &s->unitMetalMake.left, "UNITMETALMAKE", s->name);
        ReadSideRect(&parser, &s->unitMetalUse.left, "UNITMETALUSE", s->name);
        ReadSideRect(&parser, &s->unitEnergyMake.left, "UNITENERGYMAKE", s->name);
        ReadSideRect(&parser, &s->unitEnergyUse.left, "UNITENERGYUSE", s->name);
        ReadSideRect(&parser, &s->missionText.left, "MISSIONTEXT", s->name);
        ReadSideRect(&parser, &s->unitName2.left, "UNITNAME2", s->name);
        ReadSideRect(&parser, &s->damageBar2.left, "DAMAGEBAR2", s->name);
        ReadSideRect(&parser, &s->nameBlock.left, "NAME", s->name);
        ReadSideRect(&parser, &s->description.left, "DESCRIPTION", s->name);
        {
            int n = 3;
            int k = 1;
            while (n) {
                sprintf(name, "RELOAD%d", k);
                ReadSideRect(&parser, &s->reload[k - 1].left, name, s->name);
                n--;
                k++;
            }
        }
        side++;
        s++;
    }
    parser.Unload();
    g_game->sides = side;
}
