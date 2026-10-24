#ifndef SETTINGS_CAMPAIGN_H
#define SETTINGS_CAMPAIGN_H
class HapiBank;

class HapiBank;

HapiBank* __stdcall OpenSummaryBank(char* name);
HapiBank* __stdcall OpenHapiBank(char* name);
void __stdcall LoadCampaignRegistryName(int core, char* name);
void __stdcall SaveCampaignRegistryName(int core, char* name);
void __stdcall SaveAllMissionsSetting();
void __stdcall SaveNumSkirmishPlayers();

void __stdcall FreeSummaryBank(HapiBank* obj);
int __stdcall LoadSavedGameState(HapiBank* file);
int __stdcall SaveGameFile(char* path, char* description, int gameId);

#endif
