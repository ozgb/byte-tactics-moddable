#include "win_types.h"
#include "../../interfaces/host.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>

#pragma pack(push, 1)

#include "../util/vec3.h"

#include "../orders/mission_type.h"

struct ConsoleCommandTable;

#include "../network/net_stats.h"

struct Unit;
struct Player;
struct PlayerInfo;
struct ConstructionNanoState;
struct Gadget;
struct Layer;
struct Gui;

#include "../game/unit_resources.h"

struct ConstructionNanoState {
    char unknown_0[0x10];
    int drawFrame;
};

#include "../gui/gadget.h"
#include "../gui/layer.h"
#include "../gui/gui.h"

#include "../network/player_info.h"

#include "../network/player.h"

#include "../units/unit_def.h"
#include "../units/unit.h"
#include "../weapons/weapon_def.h"
#include "../weapons/projectile.h"

#include "../game/build_menu.h"

#include "../util/mouse_event.h"

struct CursorState {
    MouseEvent start;
    int flag;
    POINT center;
    POINT savedScroll;
};

#include "../game/game.h"
#include "../game/game_state.h"

#pragma pack(pop)

// GLOBAL: 0x511de8
extern Game* g_game;

#include "../../link/globals.h"
extern ConsoleCommandTable g_consoleCommands;
extern ConsoleCommandTable g_cheatCommands;
extern ConsoleCommandTable g_debugCommands;

Unit* __stdcall FindNextSelectedUnit(Unit*, int);
#include "../units/unit_commands.h"
#include "../map/radar.h"
#include "../graphics/display.h"
#include "../gui/keyboard.h"
void HideSoftwareCursor();
void ShowSoftwareCursor();

#include "../util/commands.h"
void __stdcall SetDefaultCommandHandler(void (__stdcall* callback)(int), int mask);
#include "../game/console_commands.h"

void __stdcall IssueOrCancelOrder(MissionType kind, int remove, Unit* owner,
                            int id, Vec3* pos, int typeId, int amount);
int __stdcall CanBuildAt(UnitDef* type, Point16 cell, int a, Player* player);
#include "../graphics/movies.h"
#include "../map/terrain.h"
Gadget* __stdcall FindGadgetOrNull(Gadget* entries, char* name);
#include "../orders/order_list.h"
int __stdcall FindGadgetIndexBySubstring(int value, char* name);
int __stdcall FindGadgetIndexBySubstring(Gadget* entries, char* name);
void __stdcall SetGadgetStatus(Gui* menu, int index, int value);
void __stdcall PlaySoundByName(char* name, int broadcast);
void __stdcall AdjustBuildCount(MissionType kind, Unit* unit, int id, int count);
#include "selection.h"
void __stdcall SetGadgetActive(Gui* menu, int index, char value);
#include "../gui/dialogs.h"
#include "control_panel.h"
#include "../gui/dialogs.h"
#include "../game/data_files.h"
#include "../files/files_api.h"
void __stdcall AttachUnitToPiece(Unit* unit, Unit* target, char p3, char p4);
#include "../network/net_game.h"

static inline Point16 WorldToCell(Vec3 v, Point16 origin)
{
    Point16 c;
    c.x = (v.x - (origin.x << 19) + 0x80000) >> 20;
    c.y = (v.z - (origin.y << 19) + 0x80000) >> 20;
    return c;
}

static inline void CellToWorld(Point16 origin, Point16 c, Vec3* v)
{
    v->x = (origin.x + c.x * 2) << 19;
    v->z = (origin.y + c.y * 2) << 19;
}

// FUNCTION: 0x419560
void InitCommands()
{
    DAT_00511c20 = g_game->ticks;
    g_messageBytesReceived = 0;
    g_messageBytesSent = 0;
    MessageByteCounter* p = (MessageByteCounter*)&g_messageBytesByType[1];
    for (int i = 0; i < 44; i++) {
        g_messageCountByType[i + 1][0] = 0;
        g_messageCountByType[i + 1][1] = 0;
        g_messageBytesByType[i + 1][0] = 0;
        p->b = 0;
        p++;
    }
    g_packetsSent = 0;
    g_packetBytesSent = 0;
    g_packetsReceived = 0;
    g_packetBytesReceived = 0;
    RegisterCommands((ConsoleCommand *)&g_consoleCommands);
    RegisterCommands((ConsoleCommand *)&g_cheatCommands);
    RegisterCommands((ConsoleCommand *)&g_debugCommands);
    SetDefaultCommandHandler((void (__stdcall *)(int))DefaultCommandHandler, 4);
}

// FUNCTION: 0x4195f0
void __stdcall CopyMenuEntryName(char* dest, unsigned short index)
{
    strncpy(dest, ((UnitDef *)g_game->defs.get())[index].unitname, 0x20);
    dest[0x1f] = 0;
}

// FUNCTION: 0x419630
int __stdcall MenuEntryNameContains(Gadget* entries, char* text, int index)
{
    char name[32];
    GetGadgetName(entries, name, index);
    return strstr(name, text) != 0;
}

// FUNCTION: 0x419670
void __stdcall IssueMobileBuildOrders(MouseEvent* arg)
{
    unsigned int remove = (arg->keyFlags >> 2) & 1;
    unsigned short index = g_game->buildTypeIndex;
    UnitDef* def = &((UnitDef *)g_game->defs.get())[index];
    Vec3 pos = g_game->pos;
    Point16 origin = def->origin;
    Point16 cell = WorldToCell(pos, origin);
    CellToWorld(origin, cell, &pos);
    pos.y = g_game->boxStartHeight << 16;

    unsigned char team = g_game->localPlayer;
    Player* p = &g_game->players[team];
    for (Unit* u = p->unitsBegin; u <= p->unitsEnd; u++) {
        if ((u->flags & 0x10) && (((UnitDef*)u->def.get())->flags1 & 0x40)) {
            if (!(((UnitDef*)u->def.get())->flags1 & 0x800)) {
                IssueOrCancelOrder("MOBILEBUILD", remove, u, 0, &pos, index, 0);
            } else {
                if (((UnitDef*)u->def.get())->flying) {
                    IssueOrCancelOrder("VTOL_MOBILEBUILD", remove, u, 0, &pos, index, 0);
                }
            }
        }
    }
}

// FUNCTION: 0x4197d0
int UpdatePlacementGhostValidity(void)
{
    UnitDef* item = &((UnitDef *)g_game->defs.get())[g_game->buildTypeIndex];
    Point16 cell = WorldToCell(g_game->pos, item->origin);
    g_game->boxStartX = cell.x << 4;
    g_game->boxStartZ = cell.y << 4;
    Point16 origin = item->origin;
    g_game->boxEndX = (origin.x << 4) + g_game->boxStartX;
    g_game->boxEndZ = (origin.y << 4) + g_game->boxStartZ;
    g_game->inputFlags_2cc6.bits.b6 = CanBuildAt(item, cell, 0, &g_game->players[g_game->localPlayer]);
    unsigned char r;
    if (g_game->inputFlags_2cc6.value & 0x40)
        r = GetBuildSiteHeight();
    else
        r = GetFootprintHeight(item, cell);
    g_game->boxStartHeight = r;
    g_game->boxEndHeight = r;
    return (g_game->inputFlags_2cc6.value >> 6) & 1;
}

// FUNCTION: 0x419940
void __stdcall SetBuildCountText(Gui* obj, unsigned short index, int n)
{
    Gadget* e = FindGadgetOrNull(obj->layer->entries, ((UnitDef *)g_game->defs.get())[index].unitname);
    if (e) {
        if (n)
            sprintf(e->u.text, "+%d", n);
        else
            e->u.text[0] = 0;
    }
}

// FUNCTION: 0x4199b0
void __stdcall RefreshBuildCountTexts(Gui* menu, Unit* unit)
{
    Gadget* e = menu->layer->entries;
    int count = e->u.count;
    e++;
    for (int i = 1; i < count + 1; i++) {
        char* text = e->u.text;
        if (e->type == 1) {
            if (e->commonAttribs & 4) {
                unsigned short v = FindUnitTypeId(e->name);
                if (v != 0) {
                    int r = SumQueuedBuildCount(unit, v);
                    if (r != 0)
                        sprintf(text, "+%d", r);
                    else
                        text[0] = 0;
                }
            } else if (e->commonAttribs & 8) {
                int n = unit->weapons[0].stockpile;
                int r = SumQueuedBuildCount(unit, 0);
                text[0] = 0;
                if (n != 0)
                    sprintf(text, "%d", n);
                if (r != 0)
                    sprintf(text + strlen(text), " +%d", r);
            }
        }
        e++;
    }
    MarkChanged(&g_game->gui);
}

// FUNCTION: 0x419ac0
void __stdcall UpdateOnOffButton(Unit* unit)
{
    int index = FindGadgetIndexBySubstring((int)g_game->gui.layer->entries, "ONOFF");
    if (index != -1) {
        SetGadgetStatus(&g_game->gui, index, unit->view10e.onOff);
    }
}

// FUNCTION: 0x419b00
void __stdcall QueueBuildOrder(char* name, Unit* unit, int count)
{
    if (unit->playerIndex == g_game->playerIndex) {
        if (count > 0)
            PlaySoundByName("addbuild", 0);
        else
            PlaySoundByName("subbuild", 0);
    }
    if (strstr(name, "MAKENUKE") != 0 || strstr(name, "MAKEANTI") != 0) {
        AdjustBuildCount(MissionType("BUILDWEAPON"), unit, 0, count);
        return;
    }
    unsigned short id = FindUnitTypeId(name);
    if (id == 0)
        return;
    int mobile = (int)unit->motion.get();
    const char* kind = mobile ? "MOBILEBUILD" : "BUILDINGBUILD";
    AdjustBuildCount(MissionType(kind), unit, id, count);
}

// FUNCTION: 0x419bc0
void __stdcall SetOrderIntent(unsigned char mode)
{
    g_game->orderMode = mode;
    g_game->inputFlags_2cc6.value = g_game->inputFlags_2cc6.value & 0xf7;
}

// FUNCTION: 0x419be0
int __stdcall HandleOrderButtonClick(Gui* menu, Gadget* entries)
{
    char name[32];
    Gadget* e;
    if (entries[menu->hotGadgetIndex].type == 1) e = &entries[menu->hotGadgetIndex];

    GetGadgetName(entries, name, menu->hotGadgetIndex);
    if (strstr(name, "MOVE")) {
        if (e->status == 0) {
            SetOrderIntent(1);
        } else {
            SetOrderIntent(2);
        }
        PlaySoundByName("immediateorders", 0);
        return 1;
    }
    GetGadgetName(entries, name, menu->hotGadgetIndex);
    if (strstr(name, "STOP")) {
        SetOrderIntent(1);
        IssueOrderToSelection(&g_game->view, 0, "STOP", 0, 0, 0);
        PlaySoundByName("immediateorders", 0);
        return 1;
    }
    GetGadgetName(entries, name, menu->hotGadgetIndex);
    if (strstr(name, "ATTACK")) {
        if (e->status == 0) {
            SetOrderIntent(1);
        } else {
            SetOrderIntent(3);
        }
        PlaySoundByName("immediateorders", 0);
        return 1;
    }
    GetGadgetName(entries, name, menu->hotGadgetIndex);
    if (strstr(name, "BLAST")) {
        if (e->status == 0) {
            SetOrderIntent(1);
        } else {
            SetOrderIntent(4);
        }
        PlaySoundByName("immediateorders", 0);
        return 1;
    }
    GetGadgetName(entries, name, menu->hotGadgetIndex);
    if (strstr(name, "DEFEND")) {
        if (e->status == 0) {
            SetOrderIntent(1);
        } else {
            SetOrderIntent(7);
        }
        PlaySoundByName("immediateorders", 0);
        return 1;
    }
    GetGadgetName(entries, name, menu->hotGadgetIndex);
    if (strstr(name, "REPAIR")) {
        if (e->status == 0) {
            SetOrderIntent(1);
        } else {
            SetOrderIntent(8);
        }
        PlaySoundByName("specialorders", 0);
        return 1;
    }
    GetGadgetName(entries, name, menu->hotGadgetIndex);
    if (strstr(name, "PATROL")) {
        if (e->status == 0) {
            SetOrderIntent(1);
        } else {
            SetOrderIntent(9);
        }
        PlaySoundByName("immediateorders", 0);
        return 1;
    }
    GetGadgetName(entries, name, menu->hotGadgetIndex);
    if (strstr(name, "RECLAIM")) {
        if (e->status == 0) {
            SetOrderIntent(1);
        } else {
            SetOrderIntent(0xc);
        }
        PlaySoundByName("specialorders", 0);
        return 1;
    }
    GetGadgetName(entries, name, menu->hotGadgetIndex);
    if (strstr(name, "CAPTURE")) {
        if (e->status == 0) {
            SetOrderIntent(1);
        } else {
            SetOrderIntent(0xd);
        }
        PlaySoundByName("specialorders", 0);
        return 1;
    }
    GetGadgetName(entries, name, menu->hotGadgetIndex);
    if (strstr(name, "UNLOAD")) {
        if (e->status == 0) {
            SetOrderIntent(1);
        } else {
            SetOrderIntent(5);
        }
        PlaySoundByName("specialorders", 0);
        return 1;
    }
    GetGadgetName(entries, name, menu->hotGadgetIndex);
    if (strstr(name, "LOAD")) {
        if (e->status == 0) {
            SetOrderIntent(1);
        } else {
            SetOrderIntent(6);
        }
        PlaySoundByName("immediateorders", 0);
        return 1;
    }
    return 0;
}

// FUNCTION: 0x41a120
void __stdcall RefreshOrderButtons(Unit* unit)
{
    Gui* menu = &g_game->gui;
    int layer = (int)g_game->gui.layer->entries;
    int index;

    index = FindGadgetIndexBySubstring(layer, "BUILD");
    if (index != -1) {
        if (unit && ((UnitDef*)unit->def.get())->buildMenuPageCount) {
            SetGadgetStatus(menu, index, unit->menu.menuOpen);
        } else {
            SetGrayedOut(menu, index, 1);
        }
    }
    index = FindGadgetIndexBySubstring(layer, "ORDERS");
    if (index != -1) {
        if (unit && ((UnitDef*)unit->def.get())->buildMenuPageCount) {
            SetGadgetStatus(menu, index, !unit->menu.menuOpen);
        } else {
            SetGrayedOut(menu, index, 1);
        }
    }
    index = FindGadgetIndexBySubstring(layer, "CLOAK");
    if (index != -1) {
        if (g_game->ordersPanelFlags_37ebe_6.bits.cloak == 3) {
            SetGrayedOut(menu, index, 1);
        } else {
            SetGadgetStatus(menu, index, g_game->ordersPanelFlags_37ebe_6.bits.cloak);
        }
    }
    index = FindGadgetIndexBySubstring(layer, "ONOFF");
    if (index != -1) {
        if (g_game->ordersPanelFlags_37ebe_6.bits.onOff == 3) {
            SetGrayedOut(menu, index, 1);
        } else {
            SetGadgetStatus(menu, index, g_game->ordersPanelFlags_37ebe_6.bits.onOff);
        }
    }
    index = FindGadgetIndexBySubstring(layer, "MOVEORD");
    if (index != -1) {
        if (g_game->ordersPanelFlags_37ebe_6.bits.moveOrder == 4) {
            SetGrayedOut(menu, index, 1);
        } else {
            SetGadgetStatus(menu, index, g_game->ordersPanelFlags_37ebe_6.bits.moveOrder);
        }
    }
    index = FindGadgetIndexBySubstring(layer, "FIREORD");
    if (index != -1) {
        if (g_game->ordersPanelFlags_37ebe_6.bits.fireOrder == 4) {
            SetGrayedOut(menu, index, 1);
        } else {
            SetGadgetStatus(menu, index, g_game->ordersPanelFlags_37ebe_6.bits.fireOrder);
        }
    }
    if (!g_game->ordersPanelFlags_37ebe_6.bits.canMove) {
        index = FindGadgetIndexBySubstring(layer, "MOVE");
        if (index != -1) {
            SetGrayedOut(menu, index, 1);
        }
    }
    if (!g_game->ordersPanelFlags_37ebe_6.bits.canStop) {
        index = FindGadgetIndexBySubstring(layer, "STOP");
        if (index != -1) {
            SetGrayedOut(menu, index, 1);
        }
    }
    if (!g_game->ordersPanelFlags_37ebe_6.bits.canAttack) {
        index = FindGadgetIndexBySubstring(layer, "ATTACK");
        if (index != -1) {
            SetGrayedOut(menu, index, 1);
        }
    }
    if (!g_game->ordersPanelFlags_37ebe_6.bits.canDefend) {
        index = FindGadgetIndexBySubstring(layer, "DEFEND");
        if (index != -1) {
            SetGrayedOut(menu, index, 1);
        }
    }
    if (!g_game->ordersPanelFlags_37ebe_6.bits.canPatrol) {
        index = FindGadgetIndexBySubstring(layer, "PATROL");
        if (index != -1) {
            SetGrayedOut(menu, index, 1);
        }
    }
    if (!g_game->ordersPanelFlags_37ebe_6.bits.canReclaim) {
        index = FindGadgetIndexBySubstring(layer, "RECLAIM");
        if (index != -1) {
            SetGrayedOut(menu, index, 1);
        }
    }
    if (!g_game->ordersPanelFlags_37ebe_6.bits.canRepair) {
        index = FindGadgetIndexBySubstring(layer, "REPAIR");
        if (index != -1) {
            SetGrayedOut(menu, index, 1);
        }
    }
    if (!g_game->ordersPanelFlags_37ebe_6.bits.canCapture) {
        index = FindGadgetIndexBySubstring(layer, "CAPTURE");
        if (index != -1) {
            SetGrayedOut(menu, index, 1);
        }
    }
    if (!g_game->ordersPanelFlags_37ebe_6.bits.canLoad) {
        index = FindGadgetIndexBySubstring(layer, "LOAD");
        if (index != -1) {
            SetGadgetActive(menu, index, 0);
        }
        index = FindGadgetIndexBySubstring(layer, "UNLOAD");
        if (index != -1) {
            SetGrayedOut(menu, index, 1);
        }
        if (!g_game->ordersPanelFlags_37ebe_6.bits.canBlast) {
            index = FindGadgetIndexBySubstring(layer, "BLAST");
            if (index != -1) {
                SetGrayedOut(menu, index, 1);
            }
        }
    } else {
        index = FindGadgetIndexBySubstring(layer, "BLAST");
        if (index != -1) {
            SetGadgetActive(menu, index, 0);
        }
    }
}

// FUNCTION: 0x41a490
int __stdcall HandleOrdersPanelClick(Gui* menu, Gadget* entries)
{
    Unit* unit = &g_game->units[g_game->unitIndex];
    int index = menu->hotGadgetIndex;

    if (MenuEntryNameContains(entries, "MOVEORD", index)) {
        switch (g_game->ordersPanelFlags_37ebe_6.bits.moveOrder) {
        case 0:
            IssueOrderToSelection(&g_game->view, 0, "STANDING_MOVEORDER", 0, 1, 0);
            g_game->ordersPanelFlags_37ebe_6.bits.moveOrder = 1;
            break;
        case 1:
            IssueOrderToSelection(&g_game->view, 0, "STANDING_MOVEORDER", 0, 2, 0);
            g_game->ordersPanelFlags_37ebe_6.bits.moveOrder = 2;
            break;
        case 2:
        case 3:
            IssueOrderToSelection(&g_game->view, 0, "STANDING_MOVEORDER", 0, 0, 0);
            g_game->ordersPanelFlags_37ebe_6.bits.moveOrder = 0;
            break;
        }
        PlaySoundByName("setmoveorders", 0);
        SetGadgetStatus(&g_game->gui, index, g_game->ordersPanelFlags_37ebe_6.bits.moveOrder);
    } else if (MenuEntryNameContains(entries, "FIREORD", index)) {
        switch (g_game->ordersPanelFlags_37ebe_6.bits.fireOrder) {
        case 0:
            IssueOrderToSelection(&g_game->view, 0, "STANDING_FIREORDER", 0, 1, 0);
            g_game->ordersPanelFlags_37ebe_6.bits.fireOrder = 1;
            break;
        case 1:
            IssueOrderToSelection(&g_game->view, 0, "STANDING_FIREORDER", 0, 2, 0);
            g_game->ordersPanelFlags_37ebe_6.bits.fireOrder = 2;
            break;
        case 2:
        case 3:
            IssueOrderToSelection(&g_game->view, 0, "STANDING_FIREORDER", 0, 0, 0);
            g_game->ordersPanelFlags_37ebe_6.bits.fireOrder = 0;
            break;
        }
        PlaySoundByName("setfireorders", 0);
        SetGadgetStatus(&g_game->gui, index, g_game->ordersPanelFlags_37ebe_6.bits.fireOrder);
    } else if (MenuEntryNameContains(entries, "STATUS", index) || MenuEntryNameContains(entries, "ONOFF", index)) {
        switch (g_game->ordersPanelFlags_37ebe_6.bits.onOff) {
        case 0:
            IssueOrderToSelection(&g_game->view, 0, "ACTIVATE", 0, 0, 0);
            g_game->ordersPanelFlags_37ebe_6.bits.onOff = 1;
            break;
        case 1:
            IssueOrderToSelection(&g_game->view, 0, "DEACTIVATE", 0, 0, 0);
            g_game->ordersPanelFlags_37ebe_6.bits.onOff = 0;
            break;
        case 2:
            IssueOrderToSelection(&g_game->view, 0, "ACTIVATE", 0, 0, 0);
            g_game->ordersPanelFlags_37ebe_6.bits.onOff = 1;
            break;
        }
        PlaySoundByName("specialorders", 0);
        SetGadgetStatus(&g_game->gui, index, g_game->ordersPanelFlags_37ebe_6.bits.onOff);
    } else if (MenuEntryNameContains(entries, "CLOAK", index)) {
        if (g_game->ordersPanelFlags_37ebe_6.bits.cloak) {
            IssueOrderToSelection(&g_game->view, 0, "CLOAK_OFF", 0, 0, 0);
            g_game->ordersPanelFlags_37ebe_6.bits.cloak = 0;
        } else {
            IssueOrderToSelection(&g_game->view, 0, "CLOAK_ON", 0, 0, 0);
            g_game->ordersPanelFlags_37ebe_6.bits.cloak = 1;
        }
        PlaySoundByName("specialorders", 0);
        SetGadgetStatus(&g_game->gui, index, g_game->ordersPanelFlags_37ebe_6.bits.cloak);
    } else {
        return 0;
    }
    RefreshOrderButtons(g_game->unitIndex ? unit : 0);
    RenderLayer(&g_game->gui, 0x40);
    return 1;
}

// FUNCTION: 0x41a920
void __stdcall SetPrevNextGadgetNames(Unit* unit)
{
    char buf[256];
    if (((UnitDef*)unit->def.get())->buildMenuPageCount < 2) {
        sprintf(buf, "%sPREV", g_game->sideDefs[unit->player->info->side].nameprefix);
        SetGadgetActiveByName(&g_game->gui, buf, 0);
        sprintf(buf, "%sNEXT", g_game->sideDefs[unit->player->info->side].nameprefix);
        SetGadgetActiveByName(&g_game->gui, buf, 0);
    }
}

// FUNCTION: 0x41aa00
void __stdcall HandleBuildPanelClick(Gui* menu)
{
    if (menu->hotGadgetIndex != -1) {
        Gadget* entries = menu->layer->entries;
        char idName[17];
        GetGadgetName(entries, idName, menu->hotGadgetIndex);
        idName[16] = 0;
        unsigned short id = FindUnitTypeId(idName);
        Unit* unit = &g_game->units[g_game->unitIndex];
        char name[32];
        GetGadgetName(entries, name, menu->hotGadgetIndex);
        if (strstr(name, "PREV")) {
            g_game->ordersPanelFlags_37ebe_6.bits.prev = 1;
        } else if (strstr(name, "NEXT")) {
            g_game->ordersPanelFlags_37ebe_6.bits.next = 1;
        } else if (strstr(name, "ORDERS")) {
            g_game->ordersPanelFlags_37ebe_6.bits.orders = 1;
            PlaySoundByName("ordersbutton", 0);
        } else if (strstr(name, "BUILD")) {
            g_game->ordersPanelFlags_37ebe_6.bits.build = 1;
            PlaySoundByName("buildbutton", 0);
        } else if (id != 0 && ((UnitDef *)g_game->defs.get())[id].mobile == 0) {
            g_game->orderMode = 0xe;
            g_game->buildTypeIndex = id;
            PlaySoundByName("addbuild", 0);
        } else if (!HandleOrdersPanelClick(menu, entries) && !HandleOrderButtonClick(menu, entries)) {
            if (unit->selected) {
                char text[256];
                GetGadgetName(entries, text, menu->hotGadgetIndex);
                if (IsKeyDown(0xf9)) {
                    if (GetClickMode(menu) == 1)
                        QueueBuildOrder(text, unit, 5);
                    else
                        QueueBuildOrder(text, unit, -5);
                } else {
                    if (GetClickMode(menu) == 1)
                        QueueBuildOrder(text, unit, 1);
                    else
                        QueueBuildOrder(text, unit, -1);
                }
                if (unit->immobile
                    || (unit->weapons[0].weapon.get()->flags.value & 0x10000000))
                    RefreshBuildCountTexts(menu, unit);
                MarkChanged(&g_game->gui);
            }
        }
        ClearSelectedGadget(menu);
    }
}

static inline Gadget* Entries(Gadget* t)
{
    return (Gadget*)((char*)t + 2);
}

// FUNCTION: 0x41ac90
void __stdcall DisableUnavailableBuildMenuEntries(Gui* obj)
{
    Gadget* t = obj->layer->entries;
    int n = t->u.count;
    for (int i = 0; i < n; i++) {
        if (Entries(t)[i].tab & 4) {
            SetGrayedOut(obj, i, FindUnitTypeId((char*)&Entries(t)[i]) == 0);
        }
    }
}

static inline void UpdateCounts(Gui* menu)
{
    Gadget* entry = menu->layer->entries;
    int n = entry->u.count;
    int i = 0;
    if (n <= 0)
        return;
    entry = (Gadget*)((char*)entry + 2);
    do {
        if (entry[i].tab & 4)
            SetGrayedOut(menu, i, FindUnitTypeId((char*)&entry[i]) == 0);
    } while (++i < n);
}

// FUNCTION: 0x41ace0
void __stdcall OpenBuildMenuGui(Unit* unit, char* guiName, int page)
{
    if (unit->buildLeft == 0.0f) {
        Player* player = &g_game->players[g_game->localPlayer];
        int found = 0;
        char path[256];
        char name[256];
        BuildDataPath(path, "guis", guiName, "GUI");
        if (HAPI_FileLengthByName(path) == 0)
            sprintf(name, "%sDL", g_game->sideDefs[player->info->side].nameprefix);
        else
            strcpy(name, guiName);
        Layer* layer = LoadGuiLayer(&g_game->gui, name, 0);
        if (layer != 0) {
            layer->handler = HandleBuildPanelClick;
            layer->data = 0;
            for (int i = 0; i < g_game->buildListCount; i++) {
                for (int j = 0; j < g_game->buildLists[i].count; j++) {
                    BuildMenuPage* lists = g_game->buildLists;
                    BuildMenuEntry* entry = &lists[i].entries[j];
                    if ((short)((UnitDef*)unit->def.get())->id == (short)entry->typeId
                        && entry->page - 1 == page) {
                        char* src = entry->name;
                        Gadget* e = &layer->entries[entry->slot + 4];
                        e->flag_bit = 0;
                        e->commonAttribs = 4;
                        strcpy(e->name, src);
                        found = 1;
                        e->shown = 1;
                    }
                }
            }
            if (found) {
                RenderLayer(&g_game->gui, 2);
                RenderLayer(&g_game->gui, 1);
            }
            SetPrevNextGadgetNames(unit);
            RefreshOrderButtons(unit);
            RefreshBuildCountTexts(&g_game->gui, unit);
            if (unit->flags & 0x20000000) {
                int index = FindGadgetIndexBySubstring(g_game->gui.layer->entries, "ONOFF");
                if (index != -1)
                    SetGadgetStatus(&g_game->gui, index, unit->view10e.onOff);
            }
            if (page != 0)
                UpdateCounts(&g_game->gui);
            RenderLayer(&g_game->gui, 0x40);
            g_game->unitIndex = unit->id;
            g_game->unitDefIndex = unit->unitDefIndex;
        }
    }
}

// FUNCTION: 0x41b0f0
void __stdcall OpenGeneratorDialog(Unit* unit)
{
    char name[256];
    sprintf(name, "%sGEN.GUI",
            g_game->sideDefs[g_game->players[g_game->localPlayer].info->side].nameprefix);
    Layer* gadget = LoadGuiLayer(&g_game->gui, name, 0);
    if (gadget != 0) {
        gadget->handler = HandleBuildPanelClick;
        gadget->data = 0;
        RefreshOrderButtons(unit);
        RenderLayer(&g_game->gui, 0x40);
        if (unit != 0) {
            g_game->unitIndex = unit->id;
            g_game->unitDefIndex = unit->unitDefIndex;
        } else {
            g_game->unitIndex = 0;
            g_game->unitDefIndex = 0;
        }
    }
}

// FUNCTION: 0x41b200
int __stdcall GetBuildMenuPage(Unit* obj)
{
    if (obj->menu.menuOpen) {
        return obj->menu.page;
    }
    return 0;
}

// FUNCTION: 0x41b230
void __stdcall BuildEntryGuiName(char* dest, unsigned short index, int n)
{
    char name[256];
    strncpy(name, ((UnitDef *)g_game->defs.get())[index].unitname, 0x20);
    name[0x1f] = 0;
    sprintf(dest, "%s%d.GUI", name, n);
}

// FUNCTION: 0x41b2a0
Unit* GetBuildMenuFocusUnit()
{
    unsigned short index = g_game->unitIndex;
    if (index) {
        Unit* unit = &g_game->units[index];
        if (unit->unitDefIndex)
            return unit;
    }
    return 0;
}

enum UnitAbility {
    ABILITY_MOVE_ORDER = 0,
    ABILITY_FIRE_ORDER = 1,
    ABILITY_ON_OFF = 2,
    ABILITY_STOP = 3,
    ABILITY_ATTACK = 4,
    ABILITY_DEFEND = 5,
    ABILITY_PATROL = 6,
    ABILITY_MOVE = 7,
    ABILITY_LOAD = 8,
    ABILITY_REPAIR = 9,
    ABILITY_RECLAIM = 10,
    ABILITY_CAPTURE = 12,
    ABILITY_CLOAK = 13,
    ABILITY_BLAST = 14
};

static inline int HasAbility(const UnitDef* def, int ability)
{
    return (def->flags2 >> ability) & 1;
}

// FUNCTION: 0x41b2e0
void RefreshSelectionOrders()
{
    OrdersBits& orders = g_game->ordersPanelFlags_37ebe_6.bits;
    orders.refresh = 1;
    int fireOrder = 4;
    int moveOrder = 4;
    int cloak = 3;
    Unit* u;
    int onOff = 3;
    Unit* first;
    int count;
    int canMove = 0;
    int canAttack = 0;
    int canDefend = 0;
    int canPatrol = 0;
    int canLoad = 0;
    int canCapture = 0;
    int canReclaim = 0;
    int canBlast = 0;
    int canStop = 0;
    int canRepair = 0;
    Player* player = &g_game->players[g_game->localPlayer];
    if (g_game->unitIndex != 0) {
        count = 1;
        first = GetBuildMenuFocusUnit();
        if (first == 0) {
            g_game->unitIndex = 0;
            return;
        }
    } else {
        first = 0;
        count = 0;
        Unit* last = player->unitsEnd.get();
        for (u = player->unitsBegin.get(); u <= last; u++) {
            if (u->unitDefIndex == 0)
                continue;
            if (!u->selected)
                continue;
            if (first == 0)
                first = u;
            UnitDef* type = u->def;
            if (HasAbility(type, ABILITY_FIRE_ORDER)) {
                if (fireOrder == 4)
                    fireOrder = u->fireOrder;
                else if (fireOrder != u->fireOrder)
                    fireOrder = 3;
            }
            if (HasAbility(type, ABILITY_MOVE_ORDER)) {
                if (moveOrder == 4)
                    moveOrder = u->moveOrder;
                else if (moveOrder != u->moveOrder)
                    moveOrder = 3;
            }
            if (HasAbility(type, ABILITY_ON_OFF)) {
                if (onOff == 3)
                    onOff = u->activateFlags & 1;
                else if (onOff != (u->activateFlags & 1))
                    onOff = 2;
            }
            if (HasAbility(type, ABILITY_CLOAK)) {
                if (cloak == 3)
                    cloak = u->cloakOn;
                else
                    cloak = 2;
            }
            if (HasAbility(type, ABILITY_MOVE))
                canMove = 1;
            if (HasAbility(type, ABILITY_STOP))
                canStop = 1;
            if (HasAbility(type, ABILITY_ATTACK))
                canAttack = 1;
            if (HasAbility(type, ABILITY_DEFEND))
                canDefend = 1;
            if (HasAbility(type, ABILITY_PATROL))
                canPatrol = 1;
            if (HasAbility(type, ABILITY_LOAD))
                canLoad = 1;
            if (HasAbility(type, ABILITY_REPAIR))
                canRepair = 1;
            if (HasAbility(type, ABILITY_CAPTURE))
                canCapture = 1;
            if (HasAbility(type, ABILITY_RECLAIM))
                canReclaim = 1;
            if (HasAbility(type, ABILITY_BLAST))
                canBlast = 1;
            count++;
        }
        orders.fireOrder = fireOrder;
        orders.moveOrder = moveOrder;
        orders.cloak = cloak;
        orders.onOff = onOff;
        orders.canStop = canStop;
        orders.canAttack = canAttack;
        orders.canMove = canMove;
        orders.canDefend = canDefend;
        orders.canPatrol = canPatrol;
        orders.canLoad = canLoad;
        orders.canRepair = canRepair;
        orders.canReclaim = canReclaim;
        orders.canCapture = canCapture;
        orders.canBlast = canBlast;
    }
    if (count == 0) {
        PopUntilNamedLayout(0);
        orders.refresh = 0;
    } else if (count == 1 && first->def->buildMenuPageCount) {
        int page = first->menu.menuOpen ? first->menu.page : 0;
        if (page > 0 || (first->def->flags1 & 0x80000000)) {
            char gui[256];
            BuildEntryGuiName(gui, first->unitDefIndex, page);
            if ((IsScreenNamed((Gui *)&g_game->menu, gui) == 0 || g_game->unitIndex != first->id)
                && PopUntilNamedLayout(0))
                OpenBuildMenuGui(first, gui, page);
            orders.refresh = 0;
        }
    }
    if (orders.refresh) {
        if (PopUntilNamedLayout(0)) {
            if (count == 1) {
                OpenGeneratorDialog(first);
                return;
            }
            OpenGeneratorDialog(0);
        }
    }
}

// FUNCTION: 0x41b8d0
void __stdcall FinishConstruction(Unit* unit, Unit* target)
{
    if (unit && (unit->flags & 0x10000000) && ((UnitDef*)unit->def.get())->ids.offset != 0
        && target && (target->flags & 0x10000000)) {
        ((ConstructionNanoState*)target->state.get())->drawFrame = 0;
        target->buildLeft = 0;
        target->flags |= 0x2000;
        if (target->player->active != 0
            && (target->player->type == 1 || target->player->type == 2)) {
            if (target->flags & 0x20000000) {
                if (IsScreenNamed(&g_game->gui, "BUILDER.GUI"))
                    MarkChanged(&g_game->gui);
            } else {
                if (target->carrier != 0)
                    AttachUnitToPiece(target, 0, -1, 1);
            }
        }
        if (((UnitDef*)target->def.get())->b18)
            target->SetStateBits(1, 1);
        if (g_game->unitIndex == unit->id)
            RefreshBuildCountTexts(&g_game->gui, unit);
        if (((UnitDef*)target->def.get())->b24) {
            target->lastDamageType = 7;
            target->flags |= 0x4000;
        }
        if (target->player->active != 0
            && (target->player->type == 1 || target->player->type == 2))
            g_seams.network->SendBuilderLink(unit, target);
        if ((unit->flags & 0x10) || (target->flags & 0x10))
            g_game->ordersPanelFlags_37ebe_6.raw |= 0x10;
    }
}

#include "../../interfaces/seams.h"

// FUNCTION: 0x41ba60
int __stdcall AddBuildProgress(Unit* builder, Unit* unit, float amount)
{
    int result = 0;
    if (unit->buildLeft == 0.0f)
        return 0;
    if (amount >= 0.0f)
        ((unsigned char*)&unit->netDirtyFlags)[1] |= 0x80;
    if (amount == 0.0f)
        return 0;
    UnitDef* type = (UnitDef*)unit->def.get();
    float prev = unit->buildLeft;
#ifdef __clang__
    volatile float roundedNext = min(max(prev - amount / type->buildtime, 0.0f), 1.0f);
    float next = roundedNext;
#else
    float next = min(max(prev - amount / type->buildtime, 0.0f), 1.0f);
#endif
#ifdef __clang__
    volatile float roundedStep = prev - next;
    float step = roundedStep;
    volatile float roundedCharges[2] = {type->energyCost * step, type->metalCost * step};
    float energyCharge = roundedCharges[0];
    float metalCharge = roundedCharges[1];
#else
    float step = prev - next;
    float energyCharge = type->energyCost * step;
    float metalCharge = type->metalCost * step;
#endif
    int hp = (int)(prev * type->maxHealth) - (int)(next * type->maxHealth);
    if (amount < 0.0f) {
        float refund = -metalCharge;
        float& store = unit->resourceSlot.metalMake;
        if (unit->resourceSlot.player->active != 0 && unit->resourceSlot.player->type == 2) {
            switch (g_game->difficulty) {
            case 0:
                store += refund * 0.5;
                break;
            case 1:
                store += refund * 0.7;
                break;
            default:
                store += refund;
                break;
            }
        } else {
            store += refund;
        }
        unit->health = max(hp + unit->health, 0);
        unit->buildLeft = next;
        unit->flags |= 0x2000;
        if (next >= 1.0f)
            g_seams.weapons->DamageUnit(unit, unit, 30000, 9, 0);
    } else if (g_seams.economy->RequestEnergyAndMetal(&builder->resourceSlot, energyCharge, metalCharge)) {
        unit->health = min(hp + unit->health, ((UnitDef*)unit->def.get())->maxHealth);
        unit->buildLeft = next;
        unit->flags |= 0x2000;
        result = 1;
    }
    if (unit->buildLeft == 0.0f)
        FinishConstruction(builder, unit);
    return result;
}

// FUNCTION: 0x41bcd0
void __stdcall ApplyUnfinishedBuildDecay(Unit* unit, int ticks)
{
    float val = -((float)(((UnitDef*)unit->def.get())->buildtime * ticks) / ((UnitDef*)unit->def.get())->energyCost);
    AddBuildProgress(unit, unit, val);
}

// FUNCTION: 0x41bd10
int __stdcall AddRepairProgress(Unit* builder, Unit* unit, float f)
{
    int result = 0;
    UnitDef* s = (UnitDef*)unit->def.get();
    int max = s->maxHealth;
    int v = s->buildtime;
    if (unit->health >= max)
        return 0;
#ifdef __clang__
    int n1 = (int)(((double)max * f - 1.0f) / v + 1.0f);
    int n2 = (int)(((double)s->energyCost * f - 1.0f) / v + 1.0f);
#else
    int n1 = (int)((max * f - 1.0f) / v + 1.0f);
    int n2 = (int)((s->energyCost * f - 1.0f) / v + 1.0f);
#endif
    if (n1 >= 1)
        n1 = 1;
    if (n2 >= 1)
        n2 = 1;
    if (g_seams.economy->RequestEnergy(&builder->resourceSlot, &builder->resourceSlot, (float)n2)) {
        g_seams.weapons->DamageUnit(builder, unit, n1, 10, 0);
        result = 1;
    }
    return result;
}

// FUNCTION: 0x41bde0
void __stdcall StepBuildMenuPage(int toggle)
{
    unsigned short index = g_game->unitIndex;
    if (index != 0) {
        Unit* unit = &g_game->units[index];
        if (unit->unitDefIndex == 0)
            unit = 0;
        if (unit != 0) {
            if (toggle != 0) {
                if (!unit->menu.menuOpen) {
                    unit->menu.menuOpen = 1;
                    unit->menu.page = 1;
                } else if (unit->menu.page == unit->def->buildMenuPageCount - 1) {
                    unit->menu.menuOpen = 0;
                } else {
                    unit->menu.page++;
                }
            } else {
                if (unit->menu.page == unit->def->buildMenuPageCount - 1)
                    unit->menu.page = 1;
                else
                    unit->menu.page++;
                unit->menu.menuOpen = 1;
            }
            g_game->ordersPanelFlags |= 0x10;
        }
    }
    PlaySoundByName("nextbuildmenu", 0);
}

static inline unsigned int SetPage(Unit* u, unsigned int f)
{
    return (((((UnitDef*)u->def.get())->buildMenuPageCount + 0x1ff) << 23) ^ f) & 0x3800000 ^ f;
}

static inline unsigned int StepPage(unsigned int f)
{
    return (((f & 0xff800000) - 1) ^ f) & 0x3800000 ^ f;
}

// FUNCTION: 0x41bf10
void __stdcall StepBuildMenuPageBack(int toggle)
{
    unsigned short index = g_game->unitIndex;
    Unit* u;
    if (index == 0)
        goto nextbuild;
    u = &g_game->units[index];
    if (u->unitDefIndex == 0)
        u = 0;
    if (u == 0)
        goto nextbuild;

    if (toggle) {
        unsigned int f = u->flags;
        if ((f & 0x400000) == 0) {
            f |= 0x400000;
            u->flags = f;
            u->flags = SetPage(u, f);
        } else if ((f & 0x3800000) == 0x800000) {
            u->flags = f & 0xffbfffff;
        } else {
            u->flags = StepPage(f);
        }
    } else {
        unsigned int f = u->flags;
        if ((f & 0x3800000) < 0x1000000)
            u->flags = SetPage(u, f);
        else
            u->flags = StepPage(f);
        u->flags |= 0x400000;
    }
    g_game->ordersPanelFlags_37ebe_6.lo |= 0x10;

nextbuild:
    PlaySoundByName("nextbuildmenu", 0);
}

// FUNCTION: 0x41c060
void __stdcall OpenBuildMenuPage(int page)
{
    unsigned short index = g_game->unitIndex;
    if (index != 0) {
        Unit* unit = &g_game->units[index];
        if (unit->unitDefIndex == 0)
            unit = 0;
        if (unit != 0) {
            if (page < ((UnitDef*)unit->def.get())->buildMenuPageCount) {
                unit->menu.menuOpen = page > 0;
                if (page != 0)
                    unit->menu.page = page;
                g_game->ordersPanelFlags_37ebe_6.lo |= 0x10;
                PlaySoundByName("nextbuildmenu", 0);
            }
        }
    }
}

// FUNCTION: 0x41c110
void __stdcall MarkSelectionOrdersDirty(Unit* unit)
{
    if (unit->playerIndex == g_game->localPlayer && (*(unsigned char*)&unit->flags & 0x10)) {
        g_game->ordersPanelFlags_37ebe_6.raw |= 0x10;
    }
}

// FUNCTION: 0x41c150
void __stdcall UpdateBuildMenuIfFocusUnit(Unit* unit)
{
    Game* g = g_game;
    unsigned short val1 = g->unitIndex;
    unsigned short val2 = unit->id;
    if (val1 == val2) {
        RefreshBuildCountTexts(&g->gui, unit);
    }
}

// FUNCTION: 0x41c180
void DispatchOrdersPanelPageFlags()
{
    if (g_game->ordersPanelFlags_37ebe_6.bits.next) {
        g_game->ordersPanelFlags_37ebe_6.bits.next = 0;
        StepBuildMenuPage(0);
        return;
    }
    if (g_game->ordersPanelFlags_37ebe_6.bits.prev) {
        g_game->ordersPanelFlags_37ebe_6.bits.prev = 0;
        StepBuildMenuPageBack(0);
        return;
    }
    if (g_game->ordersPanelFlags_37ebe_6.bits.build) {
        g_game->ordersPanelFlags_37ebe_6.bits.build = 0;
        Unit* unit = GetBuildMenuFocusUnit();
        if (unit) {
            unit->flags |= 0x400000;
            g_game->ordersPanelFlags_37ebe_6.bits.flag_4 = 1;
        }
        return;
    }
    if (g_game->ordersPanelFlags_37ebe_6.bits.orders) {
        g_game->ordersPanelFlags_37ebe_6.bits.orders = 0;
        Unit* unit = GetBuildMenuFocusUnit();
        if (unit) {
            unit->flags &= ~0x400000;
            g_game->ordersPanelFlags_37ebe_6.bits.flag_4 = 1;
        }
    }
}

// FUNCTION: 0x41c2b0
void ResetCameraState()
{
    char saved = g_game->scrollSpeed;
    memset(&g_game->value_142f3, 0, 0x5c);
    g_game->scrollSpeed = saved;
}

// FUNCTION: 0x41c2e0
void __stdcall CycleCameraFollow(int param)
{
    Unit* val = g_game->value_142f3;
    Unit* result = FindNextSelectedUnit(val, param);
    g_game->value_142f3 = result;
}

static inline int TestBit(unsigned int* set, unsigned short n)
{
    return set[n >> 5] & (1 << (n & 0x1f));
}

// FUNCTION: 0x41c310
void FindLocalCommander()
{
    Player* p = &g_game->players[g_game->localPlayer];
    unsigned int* set = (unsigned int *)GetCategoryMask("Commander");
    for (Unit* u = p->unitsBegin; u <= p->unitsEnd; u++) {
        if (TestBit(set, u->unitDefIndex)) {
            g_game->value_142f3 = u;
        }
    }
}

// FUNCTION: 0x41c390
void __cdecl ClearCameraFollowState()
{
    (*(unsigned short*)&g_game->value_1434b) = 0;
    g_game->value_142f3 = 0;
    g_game->tracked = 0;
}

#pragma auto_inline(off)
// FUNCTION: 0x41c3c0
void ClampCameraPosition()
{
    int maxX = g_game->mapWidth - g_game->viewWidth;
    int maxY = g_game->mapHeight - g_game->viewHeight;
    if (g_game->scrollX < 0) {
        g_game->scrollX = 0;
    } else if (g_game->scrollX > maxX) {
        g_game->scrollX = maxX;
    }
    if (g_game->scrollY < 0) {
        g_game->scrollY = 0;
    } else if (g_game->scrollY > maxY) {
        g_game->scrollY = maxY;
    }
    CalcRadarViewportRect((int *)g_game->radarViewport);
}
#pragma auto_inline(on)

// FUNCTION: 0x41c450
void ClampCameraTarget()
{
    int maxX = g_game->mapWidth - g_game->viewWidth;
    int maxY = g_game->mapHeight - g_game->viewHeight;
    if (g_game->x2 < 0) {
        g_game->x2 = 0;
    } else if (g_game->x2 > maxX) {
        g_game->x2 = maxX;
    }
    if (g_game->y2 < 0) {
        g_game->y2 = 0;
    } else if (g_game->y2 > maxY) {
        g_game->y2 = maxY;
    }
}

// FUNCTION: 0x41c4c0
void __stdcall SetCameraPosition(int x, int y, int instant)
{
    if (instant != 0) {
        g_game->x2 = x;
        g_game->y2 = y;
        int maxX = g_game->mapWidth - g_game->viewWidth;
        int maxY = g_game->mapHeight - g_game->viewHeight;
        if (g_game->x2 < 0) {
            g_game->x2 = 0;
        } else if (g_game->x2 > maxX) {
            g_game->x2 = maxX;
        }
        if (g_game->y2 < 0) {
            g_game->y2 = 0;
        } else if (g_game->y2 > maxY) {
            g_game->y2 = maxY;
        }
    } else {
        g_game->scrollX = x;
        g_game->scrollY = y;
        g_game->viewDirtyFlags.lo |= 2;
        ClampCameraPosition();
        g_game->x2 = g_game->scrollX;
        g_game->y2 = g_game->scrollY;
    }
    g_game->mapFlags &= 0xfff7;
}

// FUNCTION: 0x41c5e0
void __stdcall StartScreenShake(int dx, int dy, int value)
{
    if ((g_game->uiOptionFlags_37f2f_1 & 0x10) == 0) {
        g_game->value_1432f = value;
        g_game->value_14333 = value;
        if (dx != 0) {
            g_game->sum_x = dx;
        }
        if (dy != 0) {
            g_game->sum_y = dy;
        }
        if (value > 0) {
            g_game->flags_1434e |= 1;
        }
    }
}

// FUNCTION: 0x41c640
void __stdcall AccumulateScreenShake(int dx, int dy, int value)
{
    if ((g_game->uiOptionFlags_37f2f_1 & 0x10) == 0) {
        if ((g_game->flags_1434e & 1) == 0) {
            g_game->sum_x = 0;
            g_game->sum_y = 0;
        }
        g_game->value_1432f = (g_game->value_1432f + value) / 2;
        g_game->value_14333 = g_game->value_1432f;
        g_game->sum_x += dx;
        g_game->sum_y += dy;
        if (g_game->value_1432f > 0) {
            g_game->flags_1434e |= 1;
        }
    }
}

#pragma auto_inline(off)
#include "../harness/harness.h"

// FUNCTION: 0x41c6f0
void UpdateScreenShake()
{
    if ((g_game->flags_1434e & 1) != 0) {
        int count = g_game->value_14333;
        if (count > 0) {
            int dx = g_game->sum_x * count / g_game->value_1432f;
            int dy = g_game->sum_y * count / g_game->value_1432f;
            int rx = (int)(((__int64)HarnessViewRand() * dx) / 32768) - dx / 2;
            int ry = (int)(((__int64)HarnessViewRand() * dy) / 32768) - dy / 2;
            g_game->scrollX += rx;
            g_game->scrollY += ry;
            g_game->value_14333--;
        } else {
            g_game->flags_1434e &= 0xfe;
        }
    }
}
#pragma auto_inline(on)

// FUNCTION: 0x41c7c0
void __stdcall CenterCameraOnPoint(int a, int b, int c)
{
    int cy = b - g_game->viewHeight / 2;
    int cx = a - g_game->viewWidth / 2;
    if (c != 0) {
        g_game->x2 = cx;
        g_game->y2 = cy;
        int maxX = g_game->mapWidth - g_game->viewWidth;
        int maxY = g_game->mapHeight - g_game->viewHeight;
        if (g_game->x2 < 0) {
            g_game->x2 = 0;
        } else if (g_game->x2 > maxX) {
            g_game->x2 = maxX;
        }
        if (g_game->y2 < 0) {
            g_game->y2 = 0;
        } else if (g_game->y2 > maxY) {
            g_game->y2 = maxY;
        }
    } else {
        g_game->scrollX = cx;
        g_game->scrollY = cy;
        g_game->viewDirtyFlags.lo |= 2;
        ClampCameraPosition();
        g_game->x2 = g_game->scrollX;
        g_game->y2 = g_game->scrollY;
    }
    g_game->mapFlags &= 0xfff7;
}

// FUNCTION: 0x41c8e0
void __stdcall CenterCameraOnMapPosition(Vec3* p, int param_2)
{
    int cy = (short)((p->z - (p->y >> 1)) >> 16) - g_game->viewHeight / 2;
    int cx = (short)(p->x >> 16) - g_game->viewWidth / 2;
    if (param_2 != 0) {
        g_game->x2 = cx;
        g_game->y2 = cy;
        int maxX = g_game->mapWidth - g_game->viewWidth;
        int maxY = g_game->mapHeight - g_game->viewHeight;
        if (g_game->x2 < 0) {
            g_game->x2 = 0;
        } else if (g_game->x2 > maxX) {
            g_game->x2 = maxX;
        }
        if (g_game->y2 < 0) {
            g_game->y2 = 0;
        } else if (g_game->y2 > maxY) {
            g_game->y2 = maxY;
        }
    } else {
        g_game->scrollX = cx;
        g_game->scrollY = cy;
        g_game->viewDirtyFlags.lo |= 2;
        ClampCameraPosition();
        g_game->x2 = g_game->scrollX;
        g_game->y2 = g_game->scrollY;
    }
    g_game->mapFlags &= 0xfff7;
}

static inline void SetTarget(int x, int y)
{
    g_game->x2 = x;
    g_game->y2 = y;
}

static inline int Approach(int cur, int target)
{
    int d = cur - target;
    if (d > 0) {
        if (d > 320)
            return cur + -320;
    } else {
        if (d < -320)
            return cur + 320;
    }
    return cur - d / 2;
}

// FUNCTION: 0x41ca10
void UpdateCameraFollow()
{
    Vec3* p = 0;
    if ((unsigned short)g_game->value_1434b != 0) {
        (*(unsigned short*)&g_game->value_1434b)--;
        p = &g_game->trackedPos;
    } else if (g_game->tracked != 0) {
        p = &g_game->tracked->position;
    } else if (g_game->value_142f3 != 0) {
        if ((g_game->value_142f3)->flags & 0x10000000) {
            p = &(g_game->value_142f3)->pos;
        } else {
            (*(unsigned short*)&g_game->value_1434b) = 0;
            g_game->value_142f3 = 0;
            g_game->tracked = 0;
        }
    }
    if (p != 0) {
        SetTarget((short)(p->x >> 16) - g_game->viewWidth / 2,
                  (short)((p->z - (p->y >> 1)) >> 16) - g_game->viewHeight / 2);
        ClampCameraTarget();
        g_game->mapFlags &= 0xfff7;
    }
    if (g_game->scrollX != g_game->x2) {
        g_game->viewDirtyFlags.raw |= 2;
        g_game->mapFlags &= 0xfff7;
        g_game->scrollX = Approach(g_game->scrollX, g_game->x2);
    }
    if (g_game->scrollY != g_game->y2) {
        g_game->viewDirtyFlags.raw |= 2;
        g_game->mapFlags &= 0xfff7;
        g_game->scrollY = Approach(g_game->scrollY, g_game->y2);
    }
    UpdateScreenShake();
    ClampCameraPosition();
}

// FUNCTION: 0x41cc60
void BeginMouseScroll()
{
    (*(unsigned short*)&g_game->value_1434b) = 0;
    g_game->value_142f3 = 0;
    g_game->tracked = 0;
    HideSoftwareCursor();
    CursorState* cursor = &(*(CursorState*)&g_game->cursor);
    cursor->flag = 1;
    cursor->start = g_game->view;
    cursor->savedScroll.x = g_game->scrollX / 16;
    cursor->savedScroll.y = g_game->scrollY / 16;
    cursor->center.x = GetScreenWidth() / 2;
    cursor->center.y = GetScreenHeight() / 2;
    SetCursorPosition(cursor->center.x, cursor->center.y);
}

// FUNCTION: 0x41cd20
void EndMouseScroll()
{
    CursorState* cursor = &(*(CursorState*)&g_game->cursor);
    cursor->flag = 0;
    SetCursorPosition(cursor->start.x, cursor->start.y);
    ShowSoftwareCursor();
}

static inline void ScrollTo(int x, int y)
{
    g_game->scrollX = x;
    g_game->scrollY = y;
    g_game->viewDirtyFlags.raw |= 2;
    ClampCameraPosition();
    g_game->x2 = g_game->scrollX;
    g_game->y2 = g_game->scrollY;
    g_game->mapFlags &= 0xfff7;
}

// FUNCTION: 0x41cd50
void UpdateMouseScroll()
{
    CursorState* c = &(*(CursorState*)&g_game->cursor);
    MouseEvent r = g_game->view;
    int dx = (r.x - c->center.x) / 4;
    int x = (dx + c->savedScroll.x) * 16;
    int dy = (r.y - c->center.y) / 4;
    int y = (dy + c->savedScroll.y) * 16;
    ScrollTo(x, y);
    c->savedScroll.x = g_game->scrollX / 16;
    c->savedScroll.y = g_game->scrollY / 16;
    SetCursorPosition(c->center.x, c->center.y);
    if (!(r.keyFlags & 2)) {
        CursorState* cursor = &(*(CursorState*)&g_game->cursor);
        cursor->flag = 0;
        SetCursorPosition(cursor->start.x, cursor->start.y);
        ShowSoftwareCursor();
    }
}

#include <string>

// FUNCTION: 0x41ce90
void UpdateEdgeScroll()
{
    MouseEvent mouse;
    POINT pt;
    Game* g = g_game;
    int speed = g->scrollSpeed * g->scrollScale;
    if (speed > 0x80)
        speed = 0x80;
    if (speed == 0)
        return;
    GetCurrentMouseEvent(&mouse);
    Display* d = GetDisplay();
    if (!(d->flags.byte & 2)) {
        host_cursor_get((int*)&pt.x, (int*)&pt.y);
        int w = g_game->width_37e1f;
        int h = g_game->screenHeight;
        if ((pt.x >= w || pt.y >= h) && pt.x < w + 100 && pt.y < h + 100
            && host_window_has_focus()) {
            mouse.x = pt.x;
            if (pt.x >= w)
                mouse.x = w - 1;
            mouse.y = pt.y;
            if (pt.y >= h)
                mouse.y = h - 1;
        }
    } else {
        int w = g_game->width_37e1f;
        int h = g_game->screenHeight;
        if (mouse.x >= w)
            mouse.x = w - 1;
        if (mouse.y >= h)
            mouse.y = h - 1;
    }
    int talk = IsScreenNamed(&g_game->gui, "TALK.GUI");
    int x = g_game->scrollX;
    int y = g_game->scrollY;
    if ((IsKeyDown(0xf4) && !talk) || (mouse.x == 0 && mouse.y < g_game->screenHeight))
        x -= speed;
    else if ((IsKeyDown(0xf6) && !talk) || mouse.x == g_game->width_37e1f - 1)
        x += speed;
    if ((IsKeyDown(0xf5) && !talk) || (mouse.y == 0 && mouse.x < g_game->width_37e1f))
        y -= speed;
    else if ((IsKeyDown(0xf7) && !talk) || mouse.y == g_game->screenHeight - 1)
        y += speed;
    if (g_game->scrollX != x || g_game->scrollY != y) {
        g_game->scrollX = x;
        g_game->scrollY = y;
        g_game->viewDirtyFlags.lo |= 2;
        ClampCameraPosition();
        g_game->x2 = g_game->scrollX;
        g_game->y2 = g_game->scrollY;
        g_game->mapFlags &= 0xfff7;
        (*(unsigned short*)&g_game->value_1434b) = 0;
        g_game->value_142f3 = 0;
        g_game->tracked = 0;
    }
}
