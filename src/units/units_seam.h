#ifndef UNITS_UNITS_SEAM_H
#define UNITS_UNITS_SEAM_H

struct Unit;
struct UnitsEventHeader;

int __stdcall UnitsApply(const UnitsEventHeader* event, unsigned int size);
void __stdcall AttachUnitToPiece(Unit* unit, Unit* target, char piece, char mode);

#endif
