#ifndef EXPLOSIONS_H
#define EXPLOSIONS_H
struct ExplodedPiece;

struct ExplodedPiece;
struct Explosion;
struct GafFrame;
struct Object3D;
struct PieceRec;
struct Point16;
struct Unit;
struct Vec3;

void __stdcall InitExplosions();
Object3D* __stdcall AllocExplodePieceObject();
void __stdcall FreeExplosions(void);
Explosion* __stdcall BuildCalcedExplosion(int n, int from, int to);
void __stdcall ExplodeUnitPieces(Unit* unit);
void __stdcall UpdateExplosions();
int __stdcall FindFreeExplodedPieceSlot();
void __stdcall UpdateExplodedPieces();
void __stdcall DrawExplodedPieces(void* surface);
void __stdcall DrawExplodedPieceFaces(void* surface, ExplodedPiece* obj, PieceRec* inner);
int __stdcall UpdateExplodedPiece(ExplodedPiece* obj);
int __stdcall DrawExplodedPiece(void* surface, ExplodedPiece* obj);
void __stdcall BreakPieceIntoDebris(ExplodedPiece* param);
unsigned short __stdcall FindFeatureAtPos(Vec3* pos, Point16* cell, Point16* size);

GafFrame* __stdcall BuildExplosionFrame(int size);
void __stdcall StartExplodePiece(ExplodedPiece* piece);

#endif
