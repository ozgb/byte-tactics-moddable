#ifndef UNITS_MOVEMENT_EFFECTS_H
#define UNITS_MOVEMENT_EFFECTS_H

void UnitsOnMovementTurn(const void* event, unsigned int size);
void UnitsOnMovementTilt(const void* event, unsigned int size);
void UnitsOnMovementAngles(const void* event, unsigned int size);
void UnitsOnMovementMove(const void* event, unsigned int size);
void UnitsOnMovementMoved(const void* event, unsigned int size);
void UnitsOnMovementChangeCell(const void* event, unsigned int size);
void UnitsOnMovementMoveRate(const void* event, unsigned int size);
void UnitsOnMovementSetPosition(const void* event, unsigned int size);
void UnitsOnMovementSetStateBits(const void* event, unsigned int size);
void UnitsOnMovementSfxOccupy(const void* event, unsigned int size);

#endif
