#ifndef UNITS_H
#define UNITS_H
struct ClampedPos;

struct UnitKilledPacket;

struct Player;
struct Unit;
struct Vec3;

void __stdcall ShareMapInfo(unsigned char from, unsigned char to);
void __stdcall AllocateUnitMemory(void);
int __stdcall ComparePlayers(Player* a, Player* b);
void __stdcall FreeUnitMemory(void);
void __stdcall InitUnitFromType(Unit* unit, Vec3 pos, int);
void __stdcall InitUnitScript(Unit* self);
void __stdcall CreateUnitMotion(Unit* unit);
void __stdcall InitUnit(int unitType, Vec3 pos, int, Unit* unit);
void __stdcall CreateUnitCorpse(Unit* unit, int depth, int flag);
int __stdcall IsUnitCommander(Unit* unit);
void __stdcall KillUnit(Unit* unit, int);
void __stdcall KillUnitsOfType(short id);
void __stdcall KillAllUnits(void);
void __stdcall KillPlayerUnits(unsigned char player);

void __stdcall ApplyUnitDeath(UnitKilledPacket* cmd, int local);

void __stdcall ClampPositionToMap(ClampedPos* p);

#endif
