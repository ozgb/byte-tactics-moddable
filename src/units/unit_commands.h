#ifndef UNIT_COMMANDS_H
#define UNIT_COMMANDS_H
#include "../util/angles.h"
struct Unit;
template <class T> struct Ref;

class UnitTypeSet;
struct Player;
struct Unit;

#pragma pack(push, 1)
struct OwnershipTransferPacket {
    unsigned char type;
    short unitId;
    int newOwnerNetId;
    int buildPercent;
    int hitPoints;
    Angles16 angles;
    unsigned char stockpile0;
    unsigned char stockpile1;
    unsigned char stockpile2;
};
#pragma pack(pop)

class UnitDef;

void __stdcall InitUnitCategories();
void __stdcall FreeUnitCategories();

void __cdecl CreateMissionUnits();
unsigned short __stdcall FindUnitTypeId(const char* name);
UnitTypeSet* __stdcall GetCategoryMask(char* name);
UnitTypeSet* __stdcall GetLoadCategoryMask(char* name);

void __stdcall RunInitialMission(Unit* unit, char* text, const Ref<Unit>* units, int unitCount);

#endif
