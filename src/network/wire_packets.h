#ifndef WIRE_PACKETS_H
#define WIRE_PACKETS_H

#include "../util/vec3.h"

#pragma pack(push, 1)
struct UnitKilledPacket {
    unsigned char type;
    unsigned short unitId;
    int attackerPlayerNetId;
    unsigned short attackerUnitId;
    signed char cobKilledSeverity;
    unsigned char wreckageLevel : 4;
    unsigned char killType : 4;
};

struct UnitDamagePacketOut {
    unsigned char type;
    short victimId;
    short attackerId;
    short amount;
    unsigned char headingByte;
    unsigned char damageKind;
};

struct OrderView {
    unsigned char type;
    unsigned short id1;
    unsigned short id2;
    char param;
    unsigned char param2;
};

struct WeaponFirePacket {
    unsigned char type;
    Vec3 origin;
    Vec3 aimOrVel;
    unsigned char weaponDefIndex;
    unsigned char flag : 1;
    unsigned char : 7;
    unsigned short fireHeading;
    unsigned short firePitch;
    unsigned short targetUnitId;
    unsigned short ownerUnitId;
    unsigned char weaponSlotIndex;
};

struct ProjectileDetonatePacket {
    unsigned char type;
    Vec3 aim;
    unsigned char weaponIndex;
};
#pragma pack(pop)

#endif
