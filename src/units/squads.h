#ifndef SQUADS_H
#define SQUADS_H

struct Player;
struct Unit;

void __stdcall CreateSquads(Player* owner);
void __stdcall FreeSquads(Player* owner);
void __stdcall SetUnitSquad(Unit* unit, int index);

#endif
