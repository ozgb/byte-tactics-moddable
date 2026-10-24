#ifndef UNIT_COMMANDS_H
#define UNIT_COMMANDS_H

class UnitDef;

void __stdcall InitUnitCategories();
void __stdcall FreeUnitCategories();
float __stdcall GetEnergyUse(UnitDef* def);

#endif
