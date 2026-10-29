#ifndef PARTICLES_H
#define PARTICLES_H
struct Vec3;

struct NanoTargetBounds;
struct Vec3;

struct StructRangeElement;

struct Vec3;

void __stdcall CopyPointer(int* dest, int* src);
void __stdcall InitParticlePool();
void __stdcall ExitParticlePool();
void __stdcall CreateParticleLists();
void __stdcall DestroyParticleLists();
void __stdcall UpdateParticles();
void __stdcall DrawParticles(int arg);
void __stdcall EmitThrustParticles(int posA, int posB, int period, int lifetime, short index);
void __stdcall EmitWakeParticles(int posA, int posB, int period, short index);
void __stdcall EmitBubbles(int posA, int posB, int period, short index);
void __stdcall EmitTimedBlackSmoke(Vec3* pos, int emitPeriod, int lifetime, short index);
void __stdcall EmitWeaponSmoke(Vec3* p, short index);
void __stdcall EmitTimedSubParticles(Vec3* p, short index);

StructRangeElement* __stdcall CopyStructRange(StructRangeElement* first, StructRangeElement* last, StructRangeElement* dest);

void __stdcall EmitNanoParticles(Vec3* p, void* posB, short index);
void __stdcall EmitReverseNanoParticles(NanoTargetBounds* targetBounds, Vec3* origin, short index);

void __stdcall EmitTeleportParticles(Vec3* posA, Vec3* posB, int lifetime, short index);

#endif
