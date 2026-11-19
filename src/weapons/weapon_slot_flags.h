#ifndef WEAPON_SLOT_FLAGS_H
#define WEAPON_SLOT_FLAGS_H

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
#pragma pack(pop)

#endif
