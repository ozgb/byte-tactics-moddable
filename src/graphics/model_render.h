#ifndef MODEL_RENDER_H
#define MODEL_RENDER_H
struct CountedList;

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

void __stdcall DecrementCountArgument(int, CountedList* list, int);

#endif
