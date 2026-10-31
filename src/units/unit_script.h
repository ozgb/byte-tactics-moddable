#ifndef UNIT_SCRIPT_H
#define UNIT_SCRIPT_H

#include "cob_script.h"
#include "../../interfaces/ref.h"
#include "../util/vec3.h"
#include "../graphics/model_types.h"

struct Unit;
struct GafFrame;
struct ModelPiece;

#pragma pack(push, 1)
struct PieceState {
    int object;
    int translation[3];
    unsigned short rotation[3];
    Vec3 pos;
    Ref<int> sfxOffset;
    short value;
    unsigned short visible : 1;
    unsigned short cached : 1;
    unsigned short shaded : 1;
    Ref<PieceState> sibling;
    Ref<PieceState> child;
    Ref<PieceState> parent;
};

struct ObjectState {
    int unknown_0;
    int unknown_4;
    int dirty;
    Ref<Unit> unit;
    GafFrame* bitmap;
    GafFrame* shadow;
    Vector3s pos;
    Ref<ModelPiece> root;
    PieceState pieces[1];
};
#pragma pack(pop)

class UnitScript : public CobScript {
public:
    Ref<ObjectState> state;

    virtual void SetPieceTranslation(int index, int slot, int v);
    virtual void SetPieceRotation(int index, int slot, int v);
    virtual void SetPieceVisible(int index, int flag);
    virtual void SetPieceCached(int index, int flag);
    virtual void SetPieceShaded(int index, int flag);
    virtual int GetPieceTranslation(int index, int slot);
    virtual int GetPieceRotation(int index, int slot);
    virtual int IsPieceVisible(int index);
    virtual int IsPieceCached(int index);
    virtual int IsPieceShaded(int index);
    virtual void ExplodeLegacy(int, int, int);
    virtual void PlaySoundNoop(int);
    virtual void EmitSfx(int a, int b);
    virtual void ExplodePiece(int a, unsigned int b);
    virtual void AttachUnit(unsigned short id, int a, int b);
    virtual void DropUnit(unsigned short id);
    virtual void SetUnitValue(int which, int value);
    virtual int GetUnitValue(int which, int a, int b, int c, int d);
    virtual int IsCarryingUnit(int id);
    virtual int GetTransporterId();
    void SetObjectState(int newState);
};

#endif
