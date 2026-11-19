#ifndef WEAPONS_DAMAGE_H
#define WEAPONS_DAMAGE_H

struct Unit;

#pragma pack(push, 1)
struct UnitDamagePacket {
    unsigned char type;
    unsigned short victimId;
    unsigned short attackerId;
    unsigned short amount;
    unsigned char headingByte;
    unsigned char damageKind;
};
#pragma pack(pop)

void __stdcall DamageUnit(Unit* source, Unit* target, int amount, int type, unsigned short extra);
void __stdcall ApplyUnitDamage(UnitDamagePacket* ev);
void __stdcall ParalyzeUnit(Unit* unit, int ticks);

#endif
