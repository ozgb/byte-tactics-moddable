#ifndef MODEL_RENDER_H
#define MODEL_RENDER_H

struct GafFrame;
struct Model_459200;
struct Object3do;
struct Piece_459c70;
struct Vec3;
struct Vector3s;

void __stdcall HalveFrame(GafFrame* src, GafFrame* dst);
void __stdcall SetLightVector(int, int, int);
int __fastcall RestorePieceVertices(Piece_459c70* piece, int force);
int __stdcall CountObjects(Object3do* edi);
Piece_459c70* __stdcall AddStateEntries(Model_459200* state, Object3do* obj, Piece_459c70* parent);
Piece_459c70* __stdcall LinkStateEntries(Model_459200* state, Object3do* obj, Piece_459c70* parent);
void __fastcall TransformPieces(Model_459200* owner, Piece_459c70* piece, Vector3s* angles, Vec3* delta, int deep);
void __fastcall PoseModel(Model_459200* model, Piece_459c70* piece, int force);

#endif
