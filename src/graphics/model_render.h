#ifndef MODEL_RENDER_H
#define MODEL_RENDER_H
struct ModelState;
struct Object3do;
struct UnitDrawView;

struct GafFrame;
struct ModelState;
struct Object3do;
struct ModelPiece;
struct Vec3;
struct Vector3s;

void __stdcall HalveFrame(GafFrame* src, GafFrame* dst);
void __stdcall SetLightVector(int, int, int);
int __fastcall RestorePieceVertices(ModelPiece* piece, int force);
int __stdcall CountObjects(Object3do* edi);
ModelPiece* __stdcall AddStateEntries(ModelState* state, Object3do* obj, ModelPiece* parent);
ModelPiece* __stdcall LinkStateEntries(ModelState* state, Object3do* obj, ModelPiece* parent);
void __fastcall TransformPieces(ModelState* owner, ModelPiece* piece, Vector3s* angles, Vec3* delta, int deep);
void __fastcall PoseModel(ModelState* model, ModelPiece* piece, int force);

ModelState* __stdcall CreateObjectState(Object3do* obj);
void __stdcall UpdateObjectState(UnitDrawView* unit);

#endif
