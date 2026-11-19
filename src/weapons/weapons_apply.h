#ifndef WEAPONS_APPLY_H
#define WEAPONS_APPLY_H

struct WeaponsEvent;
struct StateWriter;

int __stdcall WeaponsApply(const WeaponsEvent* event);
void __stdcall WeaponsWriteState(const StateWriter* writer);

#endif
