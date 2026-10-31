#include "sim_event_drain.h"
#include "selection.h"
#include "../game/game.h"
#include "../../link/globals.h"
#include "../sound/sound.h"
#include "../state/sim_events.h"
#include "../units/unit.h"

static void DrainUnitSpeech(const SimEvent& e)
{
    Unit* unit = &g_game->units[e.args[0]];
    if (!(unit->flags & 0x10000000))
        return;
    if ((e.flags & SIM_EVENT_SPEECH_IF_NOT_VISIBLE) && IsUnitVisible((char*)unit))
        return;
    PresentUnitSpeech(unit, (int)e.args[1], reinterpret_cast<char*>(e.args[2]));
}

void DrainSimEvents()
{
    SimEventQueue& q = SimEvents();
    while (q.head != q.tail) {
        const SimEvent e = q.records[q.head % kSimEventCapacity];
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
