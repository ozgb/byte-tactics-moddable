#ifndef MAIN_LOOP_H
#define MAIN_LOOP_H

struct Player;

void __stdcall InitFrame();
void __stdcall ReturnToMainMenuFrame();
void __stdcall MenuFrame();
void __stdcall PreBattleFrame();
void __stdcall CampaignSetupFrame();
void __stdcall SetStartingStorageBonus(Player* obj, int height, int width);
void __stdcall SpawnCommanderAtStartPos(int team, int startpos);
void __stdcall InitStartingResourcesFromSkirmish();

#endif
