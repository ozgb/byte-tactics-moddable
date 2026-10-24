#include <string.h>
#include <stdio.h>
#include <math.h>

#pragma pack(push, 1)

#include "../map/mission.h"

#include "player.h"

#include "../units/unit.h"

#include "../game/game.h"

#pragma pack(pop)

#include "../graphics/rect.h"
#include "../state/sim_globals.h"

#include "../../link/globals.h"

void __stdcall PlaySoundByName(char* name, int param_2);
#include "../gui/dialogs.h"
#include "../gui/gui.h"
#include "net_game.h"
void __stdcall ReportGameChat(char* msg);
#include "../ingame/control_panel.h"
void __stdcall SetFont(int);
void __stdcall SetTextColors(int, int);
#include "../graphics/draw.h"
void __stdcall BlitSideLogoToRect(void*, void*, Rect*, int);
void __stdcall AddMessage(char* text, int key, int value, int last);

// FUNCTION: 0x463ca0
void __stdcall AddMessage(char* text, unsigned char key, unsigned short value, char last)
{
    if (!*text)
        return;
    if (g_game->textLines == 0)
        return;
    if ((g_game->tail + 1) % g_game->textLines == g_game->head) {
        g_game->head++;
        if (g_game->head == 30)
            g_game->head = 0;
    }
    strncpy((*(ChatHudEntry (*)[30])&g_game->entries)[g_game->tail].text, text, 0x40);
    (*(ChatHudEntry (*)[30])&g_game->entries)[g_game->tail].text[0x3f] = 0;
    (*(ChatHudEntry (*)[30])&g_game->entries)[g_game->tail].time = (unsigned int)g_game->ticks;
    unsigned char c = (*(ChatHudEntry (*)[30])&g_game->entries)[g_game->tail].flags;
    (*(ChatHudEntry (*)[30])&g_game->entries)[g_game->tail].flags = (c ^ key) & 0xf ^ c;
    (*(ChatHudEntry (*)[30])&g_game->entries)[g_game->tail].unit = value;
    (*(ChatHudEntry (*)[30])&g_game->entries)[g_game->tail].player = last;
    g_game->tail++;
    if (g_game->tail == 30)
        g_game->tail = 0;
    if (last != '\n')
        PlaySoundByName("MessageArrived", 0);
    if (IsScreenNamed((Gui *)((char*)&g_game->menu_519_8b2[0]), "TIMEOUT.GUI")) {
        MarkChanged((Dialog *)((char*)&g_game->menu_519_8b2[0]));
        MarkLayerChanged((Dialog *)((char*)&g_game->menu_519_8b2[0]));
    }
}

// FUNCTION: 0x463e50
void __stdcall SendChatMessage(Player* from, char* text, int key, char* to)
{
    char buf[200];

    sprintf(buf, "<%s%s%s> %s", from->fullName, to ? "->" : DAT_005119b8,
            to ? to : DAT_005119b8, text);
    SendChatPacket(buf);
    if (g_game->net->GetGameType() == 3) {
        ReportGameChat(buf);
    }
    AddMessage(buf, key, 0, 10);
}

// FUNCTION: 0x463ef0
int ExpireOldestMessage()
{
    int result = 0;
    unsigned short i = g_game->head;
    if (g_game->tail != i
        && (*(ChatHudEntry (*)[30])&g_game->entries)[i].time + (g_game->textScroll + 1) * 30 < (unsigned int)g_game->ticks) {
        result = 1;
        g_game->head = i + 1;
        if (g_game->head == 30)
            g_game->head = 0;
    }
    return result;
}

// FUNCTION: 0x463f60
int ScrollToNextMessageUnit(void)
{
    Game* g = g_game;
    int i = g->head;
    int end = g->tail;
    while (end != i) {
        ChatHudEntry* e = &(*(ChatHudEntry (*)[30])&g->entries)[i];
        unsigned short id = e->unit;
        if (id != 0 && (e->flags & 0x10) == 0) {
            Unit* u = &g->units[id];
            if (u->flags & 0x10000000) {
                e->flags |= 0x30;
                CenterCameraOnPoint(u->xWhole, u->zWhole, 1);
                return 1;
            }
        }
        i++;
        if (i == 30)
            i = 0;
    }
    return 0;
}

// FUNCTION: 0x464060
void __stdcall DrawMessages(void* surf)
{
    int max_lines = g_game->textLines;
    int t;
    if (max_lines == 0)
        return;
    int i = g_game->tail;
    for (int n = 1; n < max_lines; n++) {
        if (i == g_game->head)
            break;
        i--;
        if (i < 0)
            i = 0x1d;
    }
    SetFont(g_game->fontComix);
    int start = GetFontHeight();
    if (g_game->tail == i)
        return;
    int y = 0x34;
    while (g_game->tail != i) {
        int show;
        switch (g_game->mode) {
        case 1:
            if (((*(ChatHudEntry (*)[30])&g_game->entries)[i].flags & 0xf) != 2)
                show = 0;
            break;
        case 2:
            show = ((*(ChatHudEntry (*)[30])&g_game->entries)[i].flags & 0xf) != 8;
            break;
        case 3:
            if (g_game->screenchat == 0) {
                switch ((*(ChatHudEntry (*)[30])&g_game->entries)[i].flags & 0xf) {
                case 1:
                case 4:
                case 8:
                    show = 1;
                    break;
                case 2:
                case 3:
                case 5:
                case 6:
                case 7:
                    show = 0;
                    break;
                default:
                    show = 0;
                    break;
                }
            } else {
                show = 1;
            }
            break;
        default:
            show = 0;
        }
        if (show) {
            if ((*(ChatHudEntry (*)[30])&g_game->entries)[i].flags & 0x20)
                SetTextColors(g_game->colors[10], 0xfe);
            else
                SetTextColors(g_game->colors[15], 0xfe);
            int height = 138;
            int id = (*(ChatHudEntry (*)[30])&g_game->entries)[i].unit_46;
            if (id != 10) {
                t = (int)(GetFontHeight() * 0.8);
                Rect r;
                r.top = y;
                r.left = 138;
                r.right = t + 138;
                r.bottom = y + t;
                height = (int)(138.0 - t * -1.5);
                BlitSideLogoToRect(surf, &g_game->players_1b63[id], &r, 0);
            }
            DrawTextClipped(surf, (char *)&(*(ChatHudEntry (*)[30])&g_game->entries)[i], height, y, -1, 0);
            y += start;
        }
        i++;
        if (i == 0x1e)
            i = 0;
    }
}
