#ifndef PLAYERS_H
#define PLAYERS_H

#include "../../interfaces/rand_stream.h"
struct Player;

struct Player;
struct Unit;

struct Gui;
struct PlayersEventHeader;
struct PlayersMapInfo;
struct StateWriter;

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
int __stdcall UpdatePlayerHead(int player);
int __stdcall UpdatePlayerBody(int player, RandStream stream);
void __stdcall UpdatePlayerTail(int player);
void __stdcall UpdatePlayersFinish();
void __stdcall InitPlayerResources();
void __stdcall LoadPlayerControllers(HapiBank* file);
void __stdcall UpdateBlink();
int __stdcall PlayersApply(const PlayersEventHeader* event, unsigned int size);
unsigned int __stdcall PlayersRegionSize(const PlayersMapInfo* info);
void __stdcall PlayersInit(const PlayersMapInfo* info, void* region, unsigned int size);
void* __stdcall PlayersShutdown();
void __stdcall PlayersWriteState(const StateWriter* writer);

int CountActiveAIPlayers();
int CountCombatPlayers();

void __stdcall LoadPlayers(HapiBank* file);
void __stdcall SavePlayers(HapiBank* file);

int __stdcall IsFootprintVisible(Player* player, int x, int y, int dx, int dy, short size);
int __stdcall IsUnitVisibleToPlayer(Player* player, Unit* u);

#endif
