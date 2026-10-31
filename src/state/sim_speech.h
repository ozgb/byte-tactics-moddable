#ifndef STATE_SIM_SPEECH_H
#define STATE_SIM_SPEECH_H

#include "../game/game.h"
#include "../../link/globals.h"
#include "../units/unit.h"
#include "sim_events.h"

inline void __stdcall RecordUnitSpeech(Unit* unit, int kind, char* text, unsigned int flags = 0)
{
    if (unit->playerIndex == g_game->playerIndex && (unit->flags & 0x10000000) && !(unit->flags & 0x4000)) {
        unsigned int args[3] = {unit->id, (unsigned int)kind, reinterpret_cast<unsigned int>(text)};
        RecordSimEvent(SIM_EVENT_UNIT_SPEECH, flags, args, 3);
    }
}

#endif
