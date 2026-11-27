#ifndef OBJECT_STATE_H
#define OBJECT_STATE_H
struct ModelState;
struct Object3do;
struct Unit;

struct ModelBuildList {
    char unknown_0[8];
    int count;
    char unknown_c[0x20 - 0xc];
    const char** names;
};

struct ModelState;
struct ModelPiece;
struct Object3do;
struct UnitDrawView;
struct Vec3;
struct Vector3s;

int __fastcall RestorePieceVertices(ModelPiece* piece, int force);
int __stdcall CountObjects(Object3do* edi);
ModelPiece* __stdcall AddStateEntries(ModelState* state, Object3do* obj, ModelPiece* parent);
ModelPiece* __stdcall LinkStateEntries(ModelState* state, Object3do* obj, ModelPiece* parent);
void __fastcall TransformPieces(ModelState* owner, ModelPiece* piece, Vector3s* angles, Vec3* delta, int deep);
void __fastcall PoseModel(ModelState* model, ModelPiece* piece, int force);
ModelState* __stdcall CreateObjectState(Object3do* obj);
unsigned int __stdcall ObjectStateSize(Object3do* obj);
void __stdcall InitObjectState(ModelState* state, Object3do* obj);
void __stdcall UpdateObjectState(UnitDrawView* unit);
void __stdcall FreeObjectState(ModelState* state);

ModelState* __stdcall CreatePlayerObjectState(Object3do* obj, ModelBuildList* list, Unit* owner);

#endif
