#ifndef PLAYERS_464290_H
#define PLAYERS_464290_H

#include "../../interfaces/rand_stream.h"

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
void __stdcall ContinueWatchingCallback(Gadget* gadget);
void __stdcall ShowContinueWatchingDialog();
void __stdcall UpdatePlayers();
void __stdcall UpdatePlayersOnStream(RandStream stream);
void __stdcall InitPlayerResources();
void __stdcall LoadPlayerControllers(HapiBank* file);
void __stdcall UpdateBlink();

#endif
