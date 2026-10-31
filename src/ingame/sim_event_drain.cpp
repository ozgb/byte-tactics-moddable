#include "game_dialogs.h"
#include "selection.h"
#include "../game/game.h"
#include "../../link/globals.h"
#include "../sound/sound.h"
#include "../state/sim_events.h"
#include "../../interfaces/services.h"
#include "../units/unit.h"

static char* const g_speechTexts[SIM_SPEECH_TEXT_COUNT] = {
    0,
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

static void DrainUnitSpeech(const SimEvent& e)
{
    Unit* unit = &g_game->units[e.args[0]];
    if (!(unit->flags & 0x10000000) || unit->playerIndex != g_game->playerIndex)
        return;
    if ((e.flags & SIM_EVENT_SPEECH_IF_NOT_VISIBLE) && IsUnitVisible((char*)unit))
        return;
    if (e.args[2] >= SIM_SPEECH_TEXT_COUNT)
        return;
    PresentUnitSpeech(unit, (int)e.args[1], g_speechTexts[e.args[2]]);
}

void DrainSimEvents()
{
    SimEventQueue& q = SimEvents();
    while (q.head != q.tail) {
        const SimEvent e = q.records[q.head % SIM_EVENT_CAPACITY];
        q.head++;
        switch (e.kind) {
        case SIM_EVENT_UNIT_SPEECH:
            DrainUnitSpeech(e);
            break;
        default:
            break;
        }
    }
}
