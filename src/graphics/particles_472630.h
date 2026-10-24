#ifndef PARTICLES_472630_H
#define PARTICLES_472630_H

struct Vec3;

void __stdcall EmitTimedBlackSmoke(Vec3* pos, int emitPeriod, int lifetime, short index);
void __stdcall EmitWeaponSmoke(Vec3* p, short index);
void __stdcall EmitTimedSubParticles(Vec3* p, short index);

#endif
