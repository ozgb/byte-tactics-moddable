#ifndef UNITS_WEAPONS_EFFECTS_H
#define UNITS_WEAPONS_EFFECTS_H

void UnitsOnWeaponsAttacked(const void* event, unsigned int size);
void UnitsOnWeaponsAttackedSpeech(const void* event, unsigned int size);
void UnitsOnWeaponsMarkDamaged(const void* event, unsigned int size);
void UnitsOnWeaponsDamageRecord(const void* event, unsigned int size);
void UnitsOnWeaponsHealth(const void* event, unsigned int size);
void UnitsOnWeaponsNotice(const void* event, unsigned int size);
void UnitsOnWeaponsBusy(const void* event, unsigned int size);

#endif
