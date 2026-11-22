#ifndef WEAPONS_APPLY_H
#define WEAPONS_APPLY_H

struct Unit;
struct WeaponsEventHeader;
struct WeaponsSlotState;
struct StateWriter;

int __stdcall WeaponsApply(const WeaponsEventHeader* event, unsigned int size);
void __stdcall WeaponsLoadState(Unit* unit, int slot, const WeaponsSlotState* state);
void __stdcall WeaponsWriteState(const StateWriter* writer);
void __stdcall WeaponsSaveState(Unit* unit, int slot, WeaponsSlotState* state);

#endif
