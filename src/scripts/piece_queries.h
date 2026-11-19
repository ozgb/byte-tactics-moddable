#ifndef PIECE_QUERIES_H
#define PIECE_QUERIES_H

struct PieceCenterObjectView;
struct SweetSpotObjectView;
struct Vec3;
struct Object;
struct Unit;

void __stdcall PoseUnitModel(Unit* unit);
Vec3 __stdcall GetPiecePosition(Object* obj, int param);
void __stdcall GetAimFromPosition(Object* obj, Vec3* out, unsigned char weapon);
void __stdcall GetNanoPiecePosition(Object* obj, Vec3* out);
void __stdcall GetPieceCenter(PieceCenterObjectView* obj, Vec3* out, int value);
void __stdcall GetSweetSpot(SweetSpotObjectView* obj, int out);

#endif
