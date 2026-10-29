#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "../network/packet_manager.h"

#pragma pack(push, 1)
struct Flags_38d75 {
    union {
        volatile unsigned short word;
        struct {
            unsigned short pad : 2;
            unsigned short bit2 : 1;
            unsigned short rest : 13;
        };
    };
};

#include "../ingame/frame_timers.h"

#include "../gui/gui.h"

struct CampaignSetupRecord {
    char unknown_0[0x96];
    char flag_96;
};

struct Gui;
struct GafSequenceSource;

#include "../network/player_info.h"

#include "game.h"
#include "mission_option_flags.h"
#include "settings.h"

struct SkirmishSlotResources {
    char unknown_0[0xc];
    int metal;
    int energy;
    char unknown_14[4];
};

#include "../map/mission.h"
#include "../util/vec3.h"
#include "../../interfaces/seams.h"

#include "create_unit_pos.h"

#include "../network/player.h"
#include "../state/sim_globals.h"

#pragma pack(pop)

#include "../../link/globals.h"

#include "../graphics/display.h"
#include "../ingame/game_dialogs.h"
#include "../frontend/frontend.h"
#include "../frontend/frontend.h"
#include "../network/net_game.h"
unsigned int GetTicks();
#include "../ingame/keys.h"
#include "../ingame/control_panel.h"
#include "../ingame/selection.h"
#include "../ingame/info_panel.h"
#include "../graphics/pcx.h"
void __stdcall SetCursorAnimation(Gui* p, GafSequenceSource* src);
void ShowSoftwareCursor();
#include "../gui/keyboard.h"
void __cdecl LeaveNetGameCallback(int param);
void __stdcall SetCloseHandler(void (__cdecl *callback)(int), int param);
#include "main_loop.h"
#include "../harness/harness.h"
void HideSoftwareCursor();
#include "game_load.h"
void __stdcall BlitMenuLayers(Gui* sub, unsigned int* dest, int* clip);
int __stdcall BroadcastPacket(int player, void* data, int size);
#include "../network/net_game.h"
void __stdcall CreateUnit(int team, unsigned short id, CreateUnitPos pos, int a, int b,
    int c);

static inline void Charge(int bucket)
{
    FrameTimers* t = &g_game->timers;
    int now = GetMilliseconds();
    t->acc[bucket] += now - t->last;
    t->last = now;
}
#define CHARGE(bucket) Charge(bucket)

// FUNCTION: 0x496790
void MainLoopTick()
{
    FrameTimers* t = &g_game->timers;
    t->total = 0;
    for (int i = 0; i < 9; i++) {
        t->total += t->acc[i];
        t->values[i] = t->acc[i];
        t->acc[i] = 0;
    }
    if (t->total <= 0)
        t->total = 1;
    t->last = GetMilliseconds();

    if (g_game->bits_2a44.bit0) {
        UpdateFramePacing();
        if (g_game->frames_38a3b != 0) {
            RunGameSteps(1);
            CHARGE(8);
            MainLoopContinueStub();
            MainLoopContinueStub_B();
            MainLoopContinueStub_C();
            if ((*(struct Flags_38d75*)&g_game->loadingFlags).bit2) {
                if (AssignStartPositions())
                    (*(struct Flags_38d75*)&g_game->loadingFlags).word &= 0xfffb;
                CHARGE(0);
            }
            EmptyMainLoopHook();
            EmptyMainLoopHook_B();
            EmptyMainLoopHook_C();
        } else if (g_game->bits_38a51_2.flag_38a51) {
            if (g_usePacketManager)
                g_packetManager.SendAllQueued(0);
            HandleNetPackets();
            if ((int)GetTicks() > g_netProbeNextTick) {
                g_netProbeNextTick = GetTicks() + 60;
                SendProbe(GetLocalDpid(), 0);
            }
            CHARGE(0);
        }
    } else if (!g_game->bits_37ebe_1.bit0_37ebe) {
        if (!g_game->bits_38a51_2.flag_38a51) {
            UpdateFramePacing();
            if (g_game->frames_38a3b != 0) {
                RunGameSteps(0);
                CHARGE(8);
            }
        }
    }
    if (!g_game->bits_37ebe_1.bit0_37ebe) {
        HandleGameKey();
        UpdateEdgeScroll();
    }
    CollectVisibleUnitIds();
    unsigned short flags = g_game->flags_37ebe;
    if ((flags & 0x800) || (flags & 0x65) || g_game->bits_2bee_2.flags_2bee) {
        if (g_game->bits_37ebe_1.bit4_37ebe)
            g_game->unitIndex = 0;
    } else {
        if (g_game->bits_37ebe_1.bit4_37ebe) {
            g_game->bits_37ebe_1.bit4_37ebe = 0;
            RefreshSelectionOrders();
        }
    }
    DrawBattleFrame(1, 1);
    CHARGE(8);
    EmptyMainLoopHook_B();
    if (g_game->movieCaptureIndex > 0 && g_game->nextMovieFrameTick <= ((unsigned int)g_game->ticks)) {
        SaveScreenshot(g_game->text_38b53, "FRAM");
        g_game->nextMovieFrameTick += 30 / g_game->movieOutputRate;
        g_game->lastTick = GetTicks();
    }
    EmptyMainLoopHook_C();
}

// FUNCTION: 0x496a60
void InitFrame()
{
    if (((signed char)g_game->cursorMode) != 0x13) {
        g_game->cursorMode = (char)(0x13);
        SetCursorAnimation((Gui*)&g_game->gui, g_game->table[0x13]);
    }
    ShowSoftwareCursor();
    g_game->bits_2a44.bit2 = 0;
    g_game->bits_2a44.bit3 = 0;
    g_game->bits_2a44.bit0 = 0;
    g_seams.map->SetMissionType(0);
    g_game->bits_3923b.flag4_3923b = 0;
    g_game->bits_3923b.flag2_3923b = 0;
    ClearKeyQueue();
    g_game->mode_391f1 = 2;
    g_game->handler = MenuFrame;
    SetCloseHandler(LeaveNetGameCallback, 0);
}

// FUNCTION: 0x496b10
void ReturnToMainMenuFrame()
{
    EnterMainMenuState();
    g_game->bits_2a44.bit2 = 0;
    g_game->bits_2a44.bit3 = 0;
    g_game->bits_2a44.bit0 = 0;
    g_seams.map->SetMissionType(0);
    g_game->bits_3923b.flag4_3923b = 0;
    g_game->bits_3923b.flag2_3923b = 0;
    ClearKeyQueue();
    EnableKeyCommands((Dialog *)&g_game->gui);
    g_game->mode_391f1 = 2;
    g_game->handler = MenuFrame;
    SetCloseHandler(LeaveNetGameCallback, 0);
}

// FUNCTION: 0x496bb0
void MenuFrame()
{
    g_seams.units->RefreshUnitInfo();
    HarnessStartSkirmish();
    RunFrontendStateMachine();
    if (g_game->bits_2a44.bit2 && g_seams.map->GetGameType(g_game->net) == 1) {
        HideSoftwareCursor();
        g_game->mode_391f1 = 4;
        g_game->handler = CampaignSetupFrame;
        SetCloseHandler(LeaveNetGameCallback, 0);
    } else if (g_game->bits_2a44.bit2 && g_seams.map->GetGameType(g_game->net) == 2) {
        HideSoftwareCursor();
        g_game->mode_391f1 = 5;
        g_game->handler = LoadingScreenFrame;
        SetCloseHandler(LeaveNetGameCallback, 0);
    } else if (g_game->frontendState == 0x11) {
        if (g_game->bits_2a44.bit2) {
            HideSoftwareCursor();
            g_game->mode_391f1 = 3;
            g_game->handler = PreBattleFrame;
            SetCloseHandler(LeaveNetGameCallback, 0);
        }
    } else if (g_game->frontendState == 0x10) {
        if (g_game->flags_2b4c.flag) {
            if (g_game->frontendSubState == 0x12 || g_game->frontendSubState == 0x13) {
                HideSoftwareCursor();
                g_game->mode_391f1 = 3;
                g_game->handler = PreBattleFrame;
                SetCloseHandler(LeaveNetGameCallback, 0);
            }
        }
    }
    HideSoftwareCursor();
    BlitMenuLayers(&g_game->gui, 0, 0);
    ShowSoftwareCursor();
}

// FUNCTION: 0x496ce0
void PreBattleFrame()
{
    char msg;
    Player* p = &g_game->players[g_game->localPlayer];
    if (p->info->flags_97 & 1) {
        msg = 8;
        BroadcastPacket(p->id, &msg, 1);
        g_game->mode_391f1 = 5;
        g_game->handler = LoadingScreenFrame;
        SetCloseHandler(LeaveNetGameCallback, 0);
    } else if (g_game->flags_2b4c.flag) {
        if (g_netHeartbeatNextTick < GetTicks()) {
            g_netHeartbeatNextTick = GetTicks() + 0x3c;
            SendNetHeartbeat();
        }
        g_game->mode_391f1 = 5;
        g_game->handler = LoadingScreenFrame;
        SetCloseHandler(LeaveNetGameCallback, 0);
    } else {
        g_game->mode_391f1 = 5;
        g_game->handler = LoadingScreenFrame;
        SetCloseHandler(LeaveNetGameCallback, 0);
    }
}

// FUNCTION: 0x496db0
void CampaignSetupFrame()
{
    g_game->numPlayers = 2;
    g_seams.players->SetupPlayerSlot(0, 1);
    g_seams.players->SetupPlayerSlot(1, 2);
    ((CampaignSetupRecord*)g_game->p1cd5.get())->flag_96 = 1;
    g_game->mode_391f1 = 5;
    g_game->handler = LoadingScreenFrame;
    SetCloseHandler(LeaveNetGameCallback, 0);
}

// FUNCTION: 0x496e10
void __stdcall ApplyMissionOptionFlags(MissionOptionFlags* s)
{
    g_game->commanderDeath = s->value;
    g_game->bits_14281.bit2_14281 = s->flag_c;
    g_game->bits_14281.bit0_14281 = s->flag_4;
    g_game->bits_14281.bit1_14281 = s->flag_8;
}

static inline int AtLeast200(int v)
{
    if (v < 200)
        v = 200;
    return v;
}

// FUNCTION: 0x496e90
void __stdcall SetStartingStorageBonus(Player* obj, int height, int width)
{
    obj->bits_149.storage = 1;
    obj->energyStorageBonus = (float)AtLeast200(width);
    obj->metalStorageBonus = (float)AtLeast200(height);
}

// FUNCTION: 0x496ee0
void __stdcall SpawnCommanderAtStartPos(int team, int startpos)
{
    g_game->players[team].info->side = ((SkirmishOptionsTable*)g_game->options)->players[team].side;
    g_game->players[team].info->color = ((SkirmishOptionsTable*)g_game->options)->players[team].color;
    Player* p = &g_game->players[team];
    int w = ((SkirmishOptionsTable*)g_game->options)->players[team].energy;
    int v = ((SkirmishOptionsTable*)g_game->options)->players[team].metal;
    p->bits_148.started = 1;
    p->energyStorageBonus = (float)(w >= 200 ? w : 200);
    p->metalStorageBonus = (float)(v >= 200 ? v : 200);

    CreateUnitPos pos;
    if (g_seams.map->GetStartPosition(g_game->net, 0, (Vec3*)&pos, startpos)) {
        unsigned short id = g_seams.units->FindUnitTypeId(
            g_game->sideDefs[g_game->players[team].info->side].commander);
        g_seams.units->CreateUnit(team, id, *(Vec3*)&pos, 1, 1, 0);
    } else {
        char buf[128];
        sprintf(buf,
            "Error: Could not find start position number %i on the map!",
            startpos);
        FatalError(buf);
    }

    if (team == g_game->localPlayer)
        SetCameraPosition(pos.x.h.whole - g_game->viewWidth / 2,
            pos.z.h.whole - g_game->viewHeight / 2, 0);
}

static int IsPlaying(unsigned char i)
{
    if (i < 10) {
        Player* p = &g_game->players[i];
        if (p->active != 0 && (p->type == 1 || p->type == 2 || p->type == 3)
            && p->index != 10)
            return 1;
    }
    return 0;
}

static inline void __stdcall SetStartingStorageBonusInline(Player* obj, int height, int width)
{
    obj->bits_149.storage = 1;
    obj->energyStorageBonus = (float)(width >= 200 ? width : 200);
    obj->metalStorageBonus = (float)(height >= 200 ? height : 200);
}

// FUNCTION: 0x497080
void InitStartingResourcesFromSkirmish()
{
    int h = 0;
    int w = 0;
    for (int i = 0; i < 10; i++) {
        if (IsPlaying(i)) {
            SkirmishSlotResources* s = &((SkirmishSlotResources*)g_game->options)[i];
            if (s->metal > h)
                h = s->metal;
            if (s->energy > w)
                w = s->energy;
            SetStartingStorageBonusInline(&g_game->players[i], h, w);
        }
    }
}
