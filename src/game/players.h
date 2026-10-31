#ifndef PLAYERS_H
#define PLAYERS_H

#include "../../interfaces/rand_stream.h"
struct Player;

struct Player;
struct Unit;

struct Gui;

struct NetMetalSource;

class HapiBank;
struct Gadget;
struct Player;

void __stdcall ResetPlayerSlots();
void __stdcall InitPlayerSlot(Player* p);
void __stdcall LoadDefaultAIScript();
void __stdcall InitPlayers();
void __stdcall RebuildAIFeatureCells();
void __stdcall FreePlayers();
void __stdcall RefreshOwnedUnitsLineOfSight(Player* range);
void __stdcall ContinueWatchingCallback(Gui* gadget);
void __stdcall UpdatePlayers();
void __stdcall UpdatePlayersOnStream(RandStream stream);
void __stdcall InitPlayerResources();
void __stdcall LoadPlayerControllers(HapiBank* file);
void __stdcall UpdateBlink();

void __stdcall LoadPlayers(HapiBank* file);
void __stdcall SavePlayers(HapiBank* file);

float __stdcall GetNetEnergy(void*);
float __stdcall GetNetMetal(NetMetalSource*);

int __stdcall IsFootprintVisible(Player* player, int x, int y, int dx, int dy, short size);
int __stdcall IsUnitVisibleToPlayer(Player* player, Unit* u);

float __stdcall GetEnergyUsage(Player*);
float __stdcall GetMetalUsage(void*);

#endif
