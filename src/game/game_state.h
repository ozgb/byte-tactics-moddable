#ifndef GAME_STATE_H
#define GAME_STATE_H

#include "../../interfaces/rand_stream.h"

void __stdcall CreateOffscreenSurface();
void __stdcall FreeOffscreenSurface();
void __stdcall SetGameMode(int param);
void __cdecl UpdateWind();
void __cdecl UpdateWindOnStream(RandStream stream);
void __stdcall ResetGameSpeed();
void __stdcall IncreaseGameSpeed();
void __stdcall DecreaseGameSpeed();
void __stdcall LoadCdLists(void);
void __stdcall SaveCdLists();
void __stdcall ReopenCdAudio();
void __stdcall InitGame();
void __stdcall ShutdownGame(void);
void __stdcall LoadBattleAssets();
void __stdcall Force640x480Surfaces();
void __stdcall ShutdownIngameSystems();
void __stdcall ShutdownIngameAndQuit();
void __stdcall UpdateBattleHoverMode(int unused);
void __stdcall CloseTopScreenIfNotNamed();
void __stdcall FreePreviewCaches();

void __stdcall SetGameSpeed(int speed, int broadcast);
int __stdcall PopUntilNamedLayout(int force);

#endif
