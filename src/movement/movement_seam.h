#ifndef MOVEMENT_SEAM_H
#define MOVEMENT_SEAM_H

#include "../../interfaces/movement.h"
#include "../../interfaces/pathfinding.h"

int __stdcall MovementApply(const MovementEventHeader* event, unsigned int size);
void __stdcall MovementUpdateMotion(Unit* unit);
void __stdcall MovementApplyPathEvent(const PathfindingEventHeader* event, unsigned int size, const PathPoint* waypoints);
void __stdcall MovementGetMotionState(Unit* unit, MovementSavedState* out);
void __stdcall MovementReadNetState(Unit* unit, BitReader* reader);
int __stdcall MovementHasNetState(Unit* unit);
void __stdcall MovementWriteNetState(Unit* unit, BitWriter* stream);
unsigned int __stdcall MovementNewPathOrderAt(unsigned int order, const Vec3* pos);
unsigned int __stdcall MovementNewPathOrderToUnit(unsigned int order, Unit* unit);
unsigned int __stdcall MovementNewPathOrderToUnitAt(unsigned int order, Unit* unit, const Vec3* pos);
unsigned int __stdcall MovementNewPathOrderToUnitAtAngle(unsigned int order, Unit* unit, short angle);
unsigned int __stdcall MovementNewAirManeuverOrder(unsigned int order, const Vec3* from, const Vec3* to);
unsigned int __stdcall MovementNewApproachRadius(unsigned int order, int x, int y, int radius);
unsigned int __stdcall MovementNewRingApproach(unsigned int order, int x, int y, int inner, int outer);
unsigned int __stdcall MovementNewPointMarker(unsigned int order, Point16 pos, Point16 size);
unsigned int __stdcall MovementLoadTarget(int type, unsigned int order, HapiBank* file, const char* name);
void __stdcall MovementSaveTarget(unsigned int target, unsigned int order, HapiBank* file, const char* name);
int __stdcall MovementTargetType(unsigned int target);
unsigned int __stdcall MovementUnitTarget(Unit* unit);
void __stdcall MovementWriteState(const StateWriter* writer);

#endif
