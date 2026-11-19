#ifndef UNITS_WEAPONS_EFFECTS_H
#define UNITS_WEAPONS_EFFECTS_H

void UnitsOnWeaponsAttacked(const void* event);
void UnitsOnWeaponsAttackedSpeech(const void* event);
void UnitsOnWeaponsMarkDamaged(const void* event);
void UnitsOnWeaponsDamageRecord(const void* event);
void UnitsOnWeaponsHealth(const void* event);
void UnitsOnWeaponsNotice(const void* event);
void UnitsOnWeaponsBusy(const void* event);

#endif
