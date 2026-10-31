#ifndef UNITS_UNIT_SPEECH_H
#define UNITS_UNIT_SPEECH_H

#include "../game/game.h"
#include "../../link/globals.h"
#include "../state/sim_events.h"
#include "unit.h"

inline void __stdcall RecordUnitSpeech(Unit* unit, int kind, char* text, unsigned int flags = 0)
{
    if (unit->playerIndex == g_game->playerIndex && (unit->flags & 0x10000000) && !(unit->flags & 0x4000)) {
        unsigned int args[3] = {unit->id, (unsigned int)kind, reinterpret_cast<unsigned int>(text)};
        RecordSimEvent(SIM_EVENT_UNIT_SPEECH, flags, args, 3);
    }
}

inline void __stdcall RecordUnitSpeechIfNotVisible(Unit* unit, int kind, char* text)
{
    RecordUnitSpeech(unit, kind, text, SIM_EVENT_SPEECH_IF_NOT_VISIBLE);
}

#endif
