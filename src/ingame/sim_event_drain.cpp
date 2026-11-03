#include <stdio.h>
#include "game_dialogs.h"
#include "control_panel.h"
#include "selection.h"
#include "../game/game.h"
#include "../../link/globals.h"
#include "../frontend/skirmish_menu.h"
#include "../network/net_game.h"
#include "../graphics/particles.h"
#include "../gui/dialogs.h"
#include "../gui/gui.h"
#include "../game/game_state.h"
#include "../orders/box.h"
#include "../sound/sound.h"
#include "../state/sim_events.h"
#include "../../interfaces/services.h"
#include "../units/unit.h"
#include "../util/vec3.h"

void __stdcall PlaySoundByName(char* name, int flag);
void __stdcall AddMessage(char* text, int key, int value, int last);

static char* const g_speechTexts[SIM_SPEECH_TEXT_COUNT] = {
    nullptr,
    "Attacking",
    "Building",
    "Building complete",
    "Capture failed",
    "Capturing",
    "Construction stopped",
    "Construction terminated",
    "Construction terminated by hostile action",
    "Guarding",
    "I can't get there",
    "I can't reach the construction site",
    "Landing",
    "Landing aborted",
    "Landing aborted: all pads are occupied",
    "Landing aborted: no pads available",
    "Landing failed",
    "Loading",
    "Loading unit",
    "Patrolling",
    "Preparing for transport",
    "Reclaiming",
    "Reclamation failed",
    "Repair aborted.",
    "Repairing",
    "Repair mission failed",
    "Repairs unsuccessful.",
    "Ressurection failed",
    "Resurrection complete",
    "Resurrection failed",
    "Starting construction",
    "Target area was blocked",
    "That unit cannot be captured",
    "That unit cannot be reclaimed",
    "That unit is a cloud of vapor and cannot be captured",
    "Transport mission failed",
    "Unable to create any more units",
    "Unable to unload unit",
    "Unit is too heavy to transport",
    "Unit is too large to transport",
    "Unit repaired",
    "Unloading",
    "Unloading process is proceeding non-optimally",
    "Waiting for target area to clear",
};

static char* const g_soundNames[SIM_SOUND_COUNT] = {
    "Victory Condition",
    "treeburn",
};

static char* const g_messageTexts[SIM_MESSAGE_TEXT_COUNT] = {
    "Toggled ShareMetal to: %s",
    "Toggled ShareEnergy to: %s",
    "Toggled ShareMapping to: %s",
    "Toggled ShareLOS to: %s",
    "Toggled ShareRadar to: %s",
    "OK.  Will share metal if above %d",
    "OK.  Will share energy if above %d",
};

static char* const g_screenNames[SIM_SCREEN_COUNT] = {
    "BUILDER.GUI",
};

static Vec3 TakeVec3(const unsigned int* args, unsigned int at)
{
    Vec3 v;
    v.x = (int)args[at];
    v.y = (int)args[at + 1];
    v.z = (int)args[at + 2];
    return v;
}

static Unit* UnitOf(const SimEvent& e)
{
    return &g_game->units[e.args[0]];
}

static void DrainUnitSpeech(const SimEvent& e)
{
    Unit* unit = UnitOf(e);
    if (!(unit->flags & 0x10000000) || unit->playerIndex != g_game->playerIndex)
        return;
    if ((e.flags & SIM_EVENT_SPEECH_IF_NOT_VISIBLE) && IsUnitVisible((char*)unit))
        return;
    if (e.args[2] >= SIM_SPEECH_TEXT_COUNT)
        return;
    PresentUnitSpeech(unit, (int)e.args[1], g_speechTexts[e.args[2]]);
}

static void DrainSound(const SimEvent& e)
{
    if (e.args[0] < SIM_SOUND_COUNT)
        PlaySoundByName(g_soundNames[e.args[0]], (int)e.args[1]);
}

static void DrainSoundAt(const SimEvent& e)
{
    Vec3 at = TakeVec3(e.args, 1);
    PlaySoundAtHeard((int)e.args[0], &at, e.args[4]);
}

static void DrainSoundAtName(const SimEvent& e)
{
    Vec3 at = TakeVec3(e.args, 1);
    if (e.args[0] < SIM_SOUND_COUNT)
        PlaySoundAtHeard(FindSoundByName(g_soundNames[e.args[0]]), &at, e.args[4]);
}

static void DrainMessage(const SimEvent& e)
{
    if (e.args[0] >= SIM_MESSAGE_TEXT_COUNT)
        return;
    char buf[256];
    if (e.args[0] < SIM_MESSAGE_SHARE_METAL_ABOVE)
        sprintf(buf, g_messageTexts[e.args[0]], e.args[1] != 0 ? "ON" : "OFF");
    else
        sprintf(buf, g_messageTexts[e.args[0]], (int)e.args[1]);
    AddMessage(buf, 2, 0, 10);
}

static unsigned int ContinueArgs(SimEventQueue& q, const SimEvent& e, unsigned int* all)
{
    for (unsigned int i = 0; i < 6; i++)
        all[i] = e.args[i];
    if (q.head != q.tail && (q.records[q.head % SIM_EVENT_CAPACITY].flags & SIM_EVENT_CONTINUED)) {
        const SimEvent& next = q.records[q.head % SIM_EVENT_CAPACITY];
        for (unsigned int i = 0; i < 6; i++)
            all[6 + i] = next.args[i];
        q.head++;
        return 12;
    }
    return 6;
}

static void DrainNano(SimEventQueue& q, const SimEvent& e, bool reverse)
{
    unsigned int a[12] = {};
    ContinueArgs(q, e, a);
    Vec3 first = TakeVec3(a, 0);
    Vec3 second = TakeVec3(a, 3);
    Vec3 third = TakeVec3(a, 6);
    short list = (short)a[9];
    if (reverse) {
        Box box;
        box.lo = first;
        box.hi = second;
        EmitReverseNanoParticles((NanoTargetBounds*)&box, &third, list);
    } else {
        Box box;
        box.lo = second;
        box.hi = third;
        EmitNanoParticles(&first, &box, list);
    }
}

static void DrainTeleport(SimEventQueue& q, const SimEvent& e)
{
    unsigned int a[12] = {};
    ContinueArgs(q, e, a);
    Vec3 from = TakeVec3(a, 0);
    Vec3 to = TakeVec3(a, 3);
    EmitTeleportParticles(&from, &to, (int)a[6], (short)a[7]);
}

static void DrainThrust(SimEventQueue& q, const SimEvent& e)
{
    unsigned int a[12] = {};
    ContinueArgs(q, e, a);
    Vec3 from = TakeVec3(a, 0);
    Vec3 to = TakeVec3(a, 3);
    EmitThrustParticles((int)&from, (int)&to, (int)a[6], (int)a[7], (short)a[8]);
}

static void DrainWake(SimEventQueue& q, const SimEvent& e, bool bubbles)
{
    unsigned int a[12] = {};
    ContinueArgs(q, e, a);
    Vec3 from = TakeVec3(a, 0);
    Vec3 to = TakeVec3(a, 3);
    if (bubbles)
        EmitBubbles((int)&from, (int)&to, (int)a[6], (short)a[7]);
    else
        EmitWakeParticles((int)&from, (int)&to, (int)a[6], (short)a[7]);
}

static void DrainCdActivity(const SimEvent& e)
{
    if (e.args[1] == g_game->localPlayer || e.args[2] == g_game->localPlayer)
        AddCdActivitySample((int)e.args[0]);
}

static void DrainScoreFlash(const SimEvent& e)
{
    if (g_game->bits_37f06_1.b7)
        FlashScorePanelKillLoss((int)e.args[0], (int)e.args[1]);
}

static void DrainGuiChanged(const SimEvent& e)
{
    if (e.args[0] < SIM_SCREEN_COUNT && IsScreenNamed(&g_game->gui, g_screenNames[e.args[0]]))
        MarkChanged(&g_game->gui);
}

static void DrainBuildQueueSound(const SimEvent& e)
{
    if (e.args[0] != g_game->playerIndex)
        return;
    char addSound[] = "addbuild";
    char subSound[] = "subbuild";
    PlaySoundByName((int)e.args[1] > 0 ? addSound : subSound, 0);
}

void DrainSimEvents()
{
    SimEventQueue& q = SimEvents();
    while (q.head != q.tail) {
        const SimEvent e = q.records[q.head % SIM_EVENT_CAPACITY];
        q.head++;
        if (e.flags & SIM_EVENT_CONTINUED)
            continue;
        switch (e.kind) {
        case SIM_EVENT_UNIT_SPEECH:
            DrainUnitSpeech(e);
            break;
        case SIM_EVENT_SOUND:
            DrainSound(e);
            break;
        case SIM_EVENT_NANO_PARTICLES:
            DrainNano(q, e, false);
            break;
        case SIM_EVENT_REVERSE_NANO_PARTICLES:
            DrainNano(q, e, true);
            break;
        case SIM_EVENT_TELEPORT_PARTICLES:
            DrainTeleport(q, e);
            break;
        case SIM_EVENT_THRUST_PARTICLES:
            DrainThrust(q, e);
            break;
        case SIM_EVENT_WAKE_PARTICLES:
            DrainWake(q, e, false);
            break;
        case SIM_EVENT_BUBBLES:
            DrainWake(q, e, true);
            break;
        case SIM_EVENT_WEAPON_SMOKE: {
            Vec3 at = TakeVec3(e.args, 0);
            EmitWeaponSmoke(&at, (short)e.args[3]);
            break;
        }
        case SIM_EVENT_TIMED_SUB_PARTICLES: {
            Vec3 at = TakeVec3(e.args, 0);
            EmitTimedSubParticles(&at, (short)e.args[3]);
            break;
        }
        case SIM_EVENT_SCREEN_SHAKE:
            AccumulateScreenShake((int)e.args[0], (int)e.args[1], (int)e.args[2]);
            break;
        case SIM_EVENT_CD_ACTIVITY:
            DrainCdActivity(e);
            break;
        case SIM_EVENT_FORCES_DESTROYED:
            AnnounceForcesDestroyed(&g_game->players[e.args[0]]);
            break;
        case SIM_EVENT_PLAYER_LEFT:
            AnnouncePlayerLeft((int)e.args[0]);
            break;
        case SIM_EVENT_PLAYER_TOOK_LEAD:
            AnnouncePlayerTookLead(g_game->players[e.args[0]].name, (int)e.args[1]);
            break;
        case SIM_EVENT_SCORE_FLASH:
            DrainScoreFlash(e);
            break;
        case SIM_EVENT_SELECTION_ORDERS_DIRTY:
            MarkSelectionOrdersDirty(UnitOf(e));
            break;
        case SIM_EVENT_BUILD_MENU_UPDATE:
            UpdateBuildMenuIfFocusUnit(UnitOf(e));
            break;
        case SIM_EVENT_DESELECT_IF_INELIGIBLE:
            DeselectIfIneligible(UnitOf(e));
            break;
        case SIM_EVENT_GUI_CHANGED_IF_SCREEN:
            DrainGuiChanged(e);
            break;
        case SIM_EVENT_WATCH_PROMPT:
            if (e.args[0] == g_game->localPlayer)
                ShowContinueWatchingDialog();
            break;
        case SIM_EVENT_WATCH_MODE_NOTICE:
            if (e.args[0] == g_game->localPlayer)
                OpenMessageBox(&g_game->gui, Translate("You are placed in watch mode because you are hosting AI players which are still alive.  If you exit, they will be terminated."), 500, 1, 1);
            break;
        case SIM_EVENT_BUILD_QUEUE_SOUND:
            DrainBuildQueueSound(e);
            break;
        case SIM_EVENT_FOCUS_COMMANDER:
            FocusCommander((int)e.args[0]);
            break;
        case SIM_EVENT_POP_TO_GAME_LAYOUT:
            PopUntilNamedLayout((int)e.args[0]);
            break;
        case SIM_EVENT_SMOKE: {
            Vec3 at = TakeVec3(e.args, 0);
            EmitSmoke(&at, (int)e.args[3], (int)e.args[4], (short)e.args[5]);
            break;
        }
        case SIM_EVENT_WHITE_SMOKE: {
            Vec3 at = TakeVec3(e.args, 0);
            EmitWhiteSmoke(&at, (short)e.args[3]);
            break;
        }
        case SIM_EVENT_BLACK_SMOKE: {
            Vec3 at = TakeVec3(e.args, 0);
            EmitBlackSmoke(&at, (short)e.args[3]);
            break;
        }
        case SIM_EVENT_SOUND_AT:
            DrainSoundAt(e);
            break;
        case SIM_EVENT_SOUND_AT_NAME:
            DrainSoundAtName(e);
            break;
        case SIM_EVENT_REMOVE_UNIT_SPEECH:
            RemoveSpeechOfUnit((int)UnitOf(e));
            break;
        case SIM_EVENT_MESSAGE:
            DrainMessage(e);
            break;
        default:
            break;
        }
    }
}
