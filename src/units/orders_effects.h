#ifndef UNITS_ORDERS_EFFECTS_H
#define UNITS_ORDERS_EFFECTS_H

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

#endif
