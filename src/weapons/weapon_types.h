#ifndef WEAPON_TYPES_H
#define WEAPON_TYPES_H

class TdfRecord;

void __stdcall LoadWeaponTypes();
void __stdcall LoadWeaponType(TdfRecord* parser);
void __stdcall FreeWeaponTypes();

#endif
