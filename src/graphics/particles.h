#ifndef PARTICLES_H
#define PARTICLES_H

void __stdcall CopyPointer(int* dest, int* src);
void __stdcall InitParticlePool();
void __stdcall ExitParticlePool();
void __stdcall CreateParticleLists();
void __stdcall UpdateParticles();
void __stdcall DrawParticles(int arg);
void __stdcall EmitThrustParticles(int posA, int posB, int period, int lifetime, short index);
void __stdcall EmitWakeParticles(int posA, int posB, int period, short index);
void __stdcall EmitBubbles(int posA, int posB, int period, short index);

#endif
