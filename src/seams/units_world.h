#ifndef SEAMS_UNITS_WORLD_H
#define SEAMS_UNITS_WORLD_H

void UnitsInbox(const void* event, unsigned int size);
void UnitsOnAiActivate(const void* event, unsigned int size);
void UnitsOnAiSetStates(const void* event, unsigned int size);
void UnitsOnEconomyCloak(const void* event, unsigned int size);
void UnitsOnPlayersKillUnit(const void* event, unsigned int size);
void UnitsOnPlayersSendUnit(const void* event, unsigned int size);
void UnitsOnPlayersCaptureUnit(const void* event, unsigned int size);
void UnitsOnWeaponsAttacked(const void* event, unsigned int size);
void UnitsOnWeaponsHit(const void* event, unsigned int size);
void UnitsOnWeaponsAttackedSpeech(const void* event, unsigned int size);
void UnitsOnWeaponsMarkDamaged(const void* event, unsigned int size);
void UnitsOnWeaponsDamageRecord(const void* event, unsigned int size);
void UnitsOnWeaponsHealth(const void* event, unsigned int size);
void UnitsOnWeaponsNotice(const void* event, unsigned int size);
void UnitsOnWeaponsBusy(const void* event, unsigned int size);
void UnitsOnOrdersSetStateBits(const void* event, unsigned int size);
void UnitsOnOrdersSetPosition(const void* event, unsigned int size);
void UnitsOnOrdersAttachToPiece(const void* event, unsigned int size);
void UnitsOnOrdersFinishConstruction(const void* event, unsigned int size);
void UnitsOnOrdersBuildDecay(const void* event, unsigned int size);
void UnitsOnOrdersRepair(const void* event, unsigned int size);
void UnitsOnOrdersUnlink(const void* event, unsigned int size);
void UnitsOnOrdersBusy(const void* event, unsigned int size);
void UnitsOnOrdersSetFlags(const void* event, unsigned int size);
void UnitsOnOrdersClearNetDirty(const void* event, unsigned int size);
void UnitsOnOrdersSetStance(const void* event, unsigned int size);
void UnitsOnOrdersSetAngles(const void* event, unsigned int size);
void UnitsOnOrdersSetBuildState(const void* event, unsigned int size);
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
