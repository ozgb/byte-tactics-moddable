#ifndef SELECTION_H
#define SELECTION_H

struct Point;
struct Unit;

void __stdcall CollectVisibleUnitIds(void);
void __stdcall ClearSelection(void);
void __stdcall SelectAllIdleUnits(void);
void __stdcall SelectUnitsOfSameTypes(void);
void __stdcall SelectAllVisibleUnits(void);
void __stdcall ClearUnitSelectFlags(void);
void __stdcall MarkLocalVisibleUnits(void);
int __stdcall SelectUnitsInBox(void*);
int __stdcall HitTestUnitScreenHull(Unit* obj, Point* p);
void __stdcall DeselectIfIneligible(Unit* unit);
Unit* __stdcall FindNextUnmarkedLocalUnit(void);
void __stdcall FocusNextLocalUnit(void);
void __stdcall FocusCommander(int);
void __stdcall CycleSelection(void);
void __stdcall CreateSquad(int);
int __stdcall SquadHasCtrlFMember(int id);
int __stdcall SquadHasReadyMember(int);

#endif
