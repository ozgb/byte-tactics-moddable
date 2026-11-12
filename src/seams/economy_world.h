#ifndef ECONOMY_WORLD_H
#define ECONOMY_WORLD_H

struct EconomyWorldView;

const EconomyWorldView* __stdcall NativeEconomyWorld();
void EconomyInbox(const void* event);

#endif
