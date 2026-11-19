#include "../util/vec3.h"

#pragma pack(push, 1)
#include "../units/unit.h"
#pragma pack(pop)

#include "../../interfaces/seams.h"
#include "../../interfaces/services.h"
#include "../../interfaces/ref.h"
#include "../game/weapons_events.h"
#include "unit_script.h"

void UnitScript::ExplodePiece(int a, unsigned int b)
{
    WeaponsPublishExplodePiece(SCHED_SCRIPTS, state->unit.get(), a, b);
}

// FUNCTION: 0x4b1eb0
void CobScript::ExplodePiece(int, unsigned int)
{
}
