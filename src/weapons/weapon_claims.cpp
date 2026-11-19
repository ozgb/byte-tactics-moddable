#include "win_types.h"
#include "../../interfaces/ref.h"

#include "../scripts/cob_script.h"

#include "../units/unit_def.h"

#include "../files/files_api.h"
#include "../map/mission.h"
#include "../sound/sound.h"
#include "../../interfaces/services.h"

struct Unit;

#include "../units/unit.h"
#include "../../interfaces/seams.h"
#include "../game/targeting_events.h"
#include "weapon_slot_flags.h"

// FUNCTION: 0x489800
void Unit::ReleaseWeapons(unsigned char index)
{
    if (index == 3) {
        this->ReleaseWeapons(0);
        this->ReleaseWeapons(1);
        index = 2;
    }
    UnitWeaponSlot* e = &weapons[index];
    if (((WeaponSlotFlagBits*)&e->flags)->bits.bit1 != 0 && ((WeaponSlotFlagBits*)&e->flags)->bits.bit4 == 0) {
        ((WeaponSlotFlagBits*)&e->flags)->bits.bit4 = 1;
        int i = index;
        TargetingPublishClear(SCHED_WEAPONS, this, i);
    }
}

// FUNCTION: 0x4898b0
void Unit::ClaimWeapons(unsigned char index)
{
    if (index == 3) {
        this->ClaimWeapons(0);
        this->ClaimWeapons(1);
        index = 2;
    }
    UnitWeaponSlot* e = &weapons[index];
    if (((WeaponSlotFlagBits*)&e->flags)->bits.bit1 != 0 && ((WeaponSlotFlagBits*)&e->flags)->bits.bit4 != 0) {
        ((WeaponSlotFlagBits*)&e->flags)->bits.bit4 = 0;
        int i = index;
        TargetingPublishClear(SCHED_WEAPONS, this, i);
    }
}
