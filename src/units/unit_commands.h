#ifndef UNIT_COMMANDS_H
#define UNIT_COMMANDS_H
class UnitTypeSet;
struct OwnershipTransferPacket;
struct Player;
struct Unit;

class UnitDef;

void __stdcall InitUnitCategories();
void __stdcall FreeUnitCategories();
float __stdcall GetEnergyUse(UnitDef* def);

void __cdecl CreateMissionUnits();
void __stdcall GiveUnitToPlayer(Unit* unit, Player* other, OwnershipTransferPacket* p);
unsigned short __stdcall FindUnitTypeId(const char* name);
UnitTypeSet* __stdcall GetCategoryMask(char* name);

#endif
