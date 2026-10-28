#ifndef UNIT_SCRIPTS_H
#define UNIT_SCRIPTS_H
struct PieceCenterObjectView;
struct SweetSpotObjectView;
struct Vec3;

struct Object;
struct Vec3;

struct Unit;

void __stdcall PoseUnitModel(Unit* unit);
int __stdcall OrderModeTakesTarget(unsigned char type);

Vec3 __stdcall GetPiecePosition(Object* obj, int param);
int __stdcall QueryWeaponPiece(Object* obj, unsigned char weapon);
void __stdcall GetAimFromPosition(Object* obj, Vec3* out, unsigned char weapon);
void __stdcall GetNanoPiecePosition(Object* obj, Vec3* out);

void __stdcall GetPieceCenter(PieceCenterObjectView* obj, Vec3* out, int value);
void __stdcall GetSweetSpot(SweetSpotObjectView* obj, int out);

#endif
