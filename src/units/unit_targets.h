#ifndef UNIT_TARGETS_H
#define UNIT_TARGETS_H
struct Unit;
struct Vec3;
struct TargetingEvent;

struct Unit;
struct UnitDamagePacket;
class UnitDef;

struct Unit;

int __stdcall UnitCanBuild(Unit* unit, short id);
void __stdcall NotifyUnitRefs(Unit* obj, int event);
void __stdcall ParalyzeUnit(Unit* unit, int ticks);
Unit* __stdcall GetWeaponTargetUnit(Unit* obj, int index);
int __stdcall ApplyTargetingEvent(const TargetingEvent* e);
int __stdcall IsTargetingUnit(Unit* list, Unit* obj);
void __stdcall AlignUnitToGround(Unit* u);

char* __stdcall MakePropList(UnitDef* obj);
void __stdcall ClearUnitRefs(Unit* obj);
void __stdcall ApplyUnitDamage(UnitDamagePacket* ev);

int __stdcall GetWeaponTargetPos(Unit* unit, Vec3* pos, int index);
Unit* __stdcall FindWeaponTarget(Unit* unit, unsigned char weapon, int useRange);
Unit* __stdcall FindBestTargetIfFireAtWill(Unit* unit);

#endif
