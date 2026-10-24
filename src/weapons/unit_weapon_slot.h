#ifndef UNIT_WEAPON_SLOT_H
#define UNIT_WEAPON_SLOT_H

#include "../units/weapon_aim_cob_cb.h"
#include "../../interfaces/ref.h"

struct WeaponDef;

struct UnitWeaponSlot {
    short aimTargetXOrUnitId;
    short aimTargetZOrUnitSentinel;
    WeaponAimCobCb aimCob;
    Ref<WeaponDef> weapon;
    int muzzleAimFromDeltaZ;
    unsigned short reloadTimer;
    short fireHeading;
    short firePitch;
    unsigned char stockpile;
    unsigned char flags;
};

#endif
