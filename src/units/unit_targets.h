#ifndef UNIT_TARGETS_H
#define UNIT_TARGETS_H
struct Unit;
struct UnitDamagePacket;
class UnitDef;

struct Unit;

int __stdcall UnitCanBuild(Unit* unit, short id);
void __stdcall NotifyUnitRefs(Unit* obj, int event);
void __stdcall ParalyzeUnit(Unit* unit, int ticks);
void __stdcall ClearWeaponTarget(Unit* unit, int index);
void __stdcall ResetWeaponTarget(Unit* obj, int index);
Unit* __stdcall GetWeaponTargetUnit(Unit* obj, int index);
int __stdcall IsTargetingUnit(Unit* list, Unit* obj);
void __stdcall AlignUnitToGround(Unit* u);

char* __stdcall MakePropList(UnitDef* obj);
void __stdcall ClearUnitRefs(Unit* obj);
void __stdcall ApplyUnitDamage(UnitDamagePacket* ev);
void __stdcall SetWeaponTargetUnit(char*, char*, int);
void __stdcall SetWeaponTargetPos(char*, int*, int);

#endif
