#ifndef UNIT_SAVE_H
#define UNIT_SAVE_H

class HapiBank;
struct Unit;

void __stdcall LoadUnits(HapiBank* file);
Unit* __stdcall LoadUnit(unsigned short id, HapiBank* file);
void __stdcall SaveUnits(HapiBank* file);

#endif
