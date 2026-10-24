#include <stdio.h>

#pragma pack(push, 1)
struct Entry_0044c420 {
    char unknown_0[0xbc];
    short firstRow;
    char unknown_be[0xc0 - 0xbe];
    short count;
    void* field_c2;
    void* rows;
    char unknown_ca[0xd6 - 0xca];
    char* flags;
};

struct Record_005129b4 {
    char unknown_0[0x52];
    int unitIndex;
    int previousMax;
    int max;
    int peerEnabled;
};

union Flags_0044c420 {
    unsigned int raw;
    struct {
        unsigned int low : 15;
        unsigned int flag : 1;
        unsigned int high : 16;
    } bits;
};

struct Item_0044c420 {
    char unknown_0[0x20];
    char name[0x225];
    Flags_0044c420 flags2;
};

struct Inner_0044c420 {
    int unknown_0;
    void* gadgets;
};

struct Menu_0044c420 {
    char unknown_0[0x18];
    Inner_0044c420* inner;
    char unknown_1c[0x60 - 0x1c];
    int current;
};

#include "../network/unit_sync.h"

#include "../game/game.h"
#pragma pack(pop)

struct Struct_004c6ac0 {
    char unknown_0[0x2c];
    unsigned char flags;
};

#include "../../link/globals.h"

Entry_0044c420* __stdcall FindGadgetChecked(void* gadgets, char* name);
int __stdcall IsCurrentGadgetNamed(Menu_0044c420* gui, char* name);
void __stdcall ClearSelectedGadget(Menu_0044c420* obj);
void __stdcall SetDescListCleanupFlag(int param_1, int param_2);
void __stdcall MarkChanged(Menu_0044c420* obj);
void __stdcall PlaySoundByName(char* name, int param_2);
void __stdcall UpdateUnitSliders(Menu_0044c420* menu, int value);
#include "multi.h"
void __stdcall FreeSurface(Struct_004c6ac0* obj);
void __cdecl GameFreeThunk(void* p);
#include "../network/net_game.h"

// FUNCTION: 0x44c420
void __stdcall HandleRestrictionsClick(Menu_0044c420* menu)
{
    int i;
    int n;
    int type;
    int* p;
    Item_0044c420* item;
    Entry_0044c420* desc;
    Entry_0044c420* pic;

    desc = FindGadgetChecked(menu->inner->gadgets, "DESCLIST");
    if (menu->current == -1) {
        i = 0;
        p = g_unitRestrictPics;
        if (desc->count > 0) {
            do {
                if (*p != 0)
                    FreeSurface((Struct_004c6ac0*)*p);
                p++;
                i++;
            } while (i < desc->count);
        }
        GameFreeThunk(desc->rows);
        GameFreeThunk(g_unitRestrictPics);
        g_unitRestrictPics = 0;
        GameFreeThunk(desc->field_c2);
        SetDescListCleanupFlag((int)menu, 1);
        if (IsHostLocal() != 0) {
            for (i = 0; i < g_game->unitDefCount; i++) {
                type = g_unitRestrictEntries[i].unitIndex;
                if (type != 0) {
                    item = &((Item_0044c420 *)g_game->defs.get())[type];
                    if (item->flags2.bits.flag) {
                    } else {
                        if (g_unitRestrictEntries[i].max == 0)
                            ((UnitSync *)g_game->sync)->DisallowUnit((Unit_0046e330*)item);
                        else
                            ((UnitSync *)g_game->sync)->AllowUnit((Unit_0046e330*)item);
                    }
                }
            }
        }
        GameFreeThunk(g_unitRestrictEntries);
        GameFreeThunk(g_unitRestrictOldCounts);
        GameFreeThunk(desc->flags);
        pic = FindGadgetChecked(menu->inner->gadgets, "PICLIST");
        if (pic != 0 && pic->rows != 0)
            GameFreeThunk(pic->rows);
        g_unitRestrictEntries = 0;
        return;
    }

    if (IsCurrentGadgetNamed(menu, "Load") != 0) {
        PlaySoundByName("Options", 0);
        OpenLoadListDialog();
        ClearSelectedGadget(menu);
        return;
    }
    if (IsCurrentGadgetNamed(menu, "Save") != 0) {
        PlaySoundByName("Options", 0);
        OpenSaveGameDialog();
        ClearSelectedGadget(menu);
        return;
    }
    if (IsCurrentGadgetNamed(menu, "Reset") != 0) {
        PlaySoundByName("Options", 0);
        for (i = 0; i < g_game->unitDefCount; i++) {
            type = g_unitRestrictEntries[i].unitIndex;
            if (type != 0) {
                if ((((Item_0044c420 *)g_game->defs.get())[type].flags2.raw & 0x10000) == 0)
                    g_unitRestrictEntries[i].max = 100;
                else
                    g_unitRestrictEntries[i].max = 0;
                if (g_unitRestrictEntries[i].max != g_unitRestrictEntries[i].previousMax) {
                    ((UnitSync *)g_game->sync)->SetUnitLimit(
                        (Unit_0046e330*)&((Item_0044c420 *)g_game->defs.get())[g_unitRestrictEntries[i].unitIndex],
                        g_unitRestrictEntries[i].max);
                }
            }
        }
        UpdateUnitSliders(menu, 0);
        MarkChanged(menu);
        ClearSelectedGadget(menu);
        return;
    }
    if (IsCurrentGadgetNamed(menu, "OK") != 0) {
        PlaySoundByName("Options", 0);
        return;
    }
    if (IsCurrentGadgetNamed(menu, "Cancel") != 0) {
        PlaySoundByName("Previous", 0);
        n = 0;
        for (i = 1; i < g_game->unitDefCount; i++) {
            if (((Item_0044c420 *)g_game->defs.get())[i].flags2.bits.flag) {
            } else if (((Item_0044c420 *)g_game->defs.get())[i].name != 0) {
                ((UnitSync *)g_game->sync)->SetUnitLimit(
                    (Unit_0046e330*)&((Item_0044c420 *)g_game->defs.get())[i], g_unitRestrictOldCounts[n]);
                n++;
            }
        }
        return;
    }
    if (menu->current != -1)
        ClearSelectedGadget(menu);
}
