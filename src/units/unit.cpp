#include "win_types.h"
#include "../../interfaces/ref.h"

#include "cob_script.h"


#include "unit_def.h"

#include "../files/files_api.h"
#include "../map/mission.h"
#include "../sound/sound.h"
#include "../state/sim_speech.h"
#include "../../interfaces/services.h"
#include "../state/sim_effects.h"


struct Unit;

#pragma pack(push, 1)
union WeaponSlotFlagBits {
    struct {
        unsigned char bit0 : 1;
        unsigned char bit1 : 1;
        unsigned char bit2 : 1;
        unsigned char bit3 : 1;
        unsigned char bit4 : 1;
        unsigned char rest : 3;
    } bits;
    unsigned char all;
};

union UnitDefCanLoadFlags {
    struct {
        unsigned int unknown_0 : 19;
        unsigned int flag19 : 1;
        unsigned int unknown_1 : 12;
    } bits;
    int all;
};

struct CargoLinkView {
    char unknown_0[0x86];
    Ref<Unit> owner;
    char unknown_8a[4];
    Ref<CargoLinkView> next;
};

#include "../game/game.h"

#include "../network/player.h"

#pragma pack(pop)

#include "../../link/globals.h"

#include "../orders/order_base.h"

#include "../orders/path_order_attach.h"

#include "unit.h"

inline unsigned char Unit::GetState() { return activateFlags; }

#include "../ingame/control_panel.h"

#include "../../interfaces/seams.h"

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
        AimTargetPair* p = (AimTargetPair*)&weapons[i];
        if (p->a != 0 || p->b != (short)0x8000) {
            p->a = 0;
            p->b = (short)0x8000;
            g_seams.units->FindScript((CobScript*)script.get(), "StartBuilding");
            g_seams.units->StartScriptWithArgs((CobScript*)script.get(), "TargetCleared", 0, 0, 1, i, 0, 0, 0);
        }
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
        AimTargetPair* p = (AimTargetPair*)&weapons[i];
        if (p->a != 0 || p->b != (short)0x8000) {
            p->a = 0;
            p->b = (short)0x8000;
            g_seams.units->FindScript((CobScript*)script.get(), "StartBuilding");
            g_seams.units->StartScriptWithArgs((CobScript*)script.get(), "TargetCleared", 0, 0, 1, i, 0, 0, 0);
        }
    }
}

// FUNCTION: 0x489960
int Unit::CanReclaim(void *param_1)
{
    void *ptr = def.get();
    unsigned int val1 = *(unsigned int *)((char *)ptr + 0x245);

    if ((val1 & 0x400) != 0) {
        unsigned int val2 = *(unsigned int *)((char *)param_1 + 0x110);

        if ((val2 & 3) != 2) {
            void *ptr2 = ((Unit*)param_1)->def.get();
            unsigned int val3 = *(unsigned int *)((char *)ptr2 + 0x245);

            if ((val3 & 0x1000) == 0) {
                return 1;
            }
        }
    }

    return 0;
}

// FUNCTION: 0x4899b0
int Unit::CanRepair(Unit* other)
{
    if (other
        && (def->flags2 & 0x200)
        && (other->health != other->def->maxHealth)
        && ((other->flags & 3) != 2)) {
        if (def->flags1 & 0x800) {
            if (!(def->flags1 & 0x200000) && other->yWhole + other->def->modelMaxYHigh < g_game->seaLevel)
                goto fail;
        }
        if (!(def->flags1 & 0x800)) {
            if (other->yWhole + other->def->modelMaxYHigh < g_game->seaLevel - def->maxwaterdepth)
                goto fail;
        }
        return 1;
    }
fail:
    return 0;
}

// FUNCTION: 0x489a70
int Unit::CountCargo()
{
    int count = 0;
    CargoLinkView* node = (CargoLinkView*)cargo.get();
    while (node) {
        if (node->owner == this)
            count++;
        node = node->next;
    }
    return count;
}

// FUNCTION: 0x489a90
int Unit::CanLoad(Unit* other)
{
    UnitDef* theirDef = other->def;
    if (((UnitDefCanLoadFlags*)&theirDef->flags2)->bits.flag19)
        return 0;
    UnitDef* ourDef = def;
    if (!(ourDef->flags2 & 0x100))
        return 0;
    if (CountCargo() >= (unsigned char)ourDef->transportcapacity)
        return 0;
    if (!other->motion)
        return 0;
    if (theirDef->footprintx > ourDef->capacity)
        return 0;
    if ((other->flags & 3) == 2)
        return 0;
    if (!(ourDef->flags1 & 0x800) && theirDef->minwaterdepth >= 0)
        return 0;
    if (other->pos.y + theirDef->modelMaxY <= (g_game->seaLevel << 16))
        return 0;
    if (other->buildLeft != 0.0f)
        return 0;
    return 1;
}

static inline int HasBit(unsigned char bits) { return 1 & bits; }

static inline int LostBits(unsigned char was, int is) { return (unsigned char)was & ~is; }

static inline unsigned char GainedBits(unsigned char was, int is) { return (unsigned char)(~was & (int)is); }

static inline unsigned char AsByte(unsigned char bits) { return (unsigned char)bits; }

void FreeCobScript(int);

// FUNCTION: 0x48b090
void Unit::SetStateBits(int mask, int set)
{
    unsigned char lost, gained, old = GetState();
    int isOne, active;
    PathOrderAttach* link;
    int now;
    if (set)
        now = old | (unsigned char)mask;
    else
        now = old & ~(mask & 0xff);
    activateFlags = (unsigned char)now;
    {
        if ((unsigned char)now != old) {
            lost = LostBits(old, now);
            unsigned char newLost = AsByte(lost);
            gained = ~old & now;
            lost = (unsigned char)newLost;
            if (gained & 1) {
                ((CobScript*)script.get())->StartScript("Activate", 0, 0);
                RecordUnitSpeech(this, 3, SIM_SPEECH_KIND_TEXT);
            }
            if (lost & 1) {
                ((CobScript*)script.get())->StartScript("Deactivate", 0, 0);
                RecordUnitSpeech(this, 4, SIM_SPEECH_KIND_TEXT);
            }
            if (gained & 8)
                ((CobScript*)script.get())->StartScript("StartBuilding", 0, 0);
            if (lost & 8)
                ((CobScript*)script.get())->StartScript("StopBuilding", 0, 0);
            if (gained & 4) {
                RecordUnitSpeech(this, 0xe, SIM_SPEECH_KIND_TEXT);
                for (link = head; link; link = link->next) {
                    if (link->value)
                        link->value->OrStatusFlags(0x10000);
                }
            }
            if (lost & 4)
                RecordUnitSpeech(this, 0xf, SIM_SPEECH_KIND_TEXT);
            RecordSelectionOrdersDirty(this);
            if (player->active != 0) {
                if (player->type == 1 || player->type == 2)
                    g_seams.network->SendActivateFlags(this);
            }
        }
    }
}
