#ifndef CAMPAIGN_MENU_H
#define CAMPAIGN_MENU_H
struct CampaignMissionObject;

struct Gadget;
struct Gui;

void __stdcall DrawBitmapBackground(char* name, int lock);
int __stdcall GetSideCount(void);
void __stdcall CountCampaignFiles();
char* __stdcall BuildSideList();
int __stdcall CampaignExists(const char* name);
int __stdcall ListCampaignFiles(void** names);
int __stdcall BuildCampaignNameList(char** out, int side);
void __stdcall StartBriefingNarration();
void __stdcall StartGlamourSound();
char* __stdcall AllocColorMarkupText(char* s);
void __stdcall InitBriefingText();
char* __stdcall SkipToPageOffset(char*, int, int);
void __stdcall DrawHelpPage();
void __cdecl ApplyCampaignSideSelection();
void __stdcall ApplyDifficultyButtons();
int __stdcall HasFewerThanThreeCampaignFiles(void);
void __stdcall HandleSingleMenuClick(Gui* gadget);
void __stdcall OpenSingleMenu();
void __stdcall FillCampaignList(char*);
void __stdcall HandleNewGameClick(Gui* menu);
void __stdcall OpenNewGameMenu(int);
void __stdcall UpdateSolarSystem(Gui* arg1, Gadget* arg2);
void __stdcall UpdatePlanet(Gui* arg1, Gadget* arg2);
void __stdcall HandleMissionBriefingClick(Gui* menu);
void __stdcall OpenMissionBriefing(void);
int __stdcall CountComputerSlots();
int __stdcall FindOpenSlot();
int __stdcall AreAllSlotsEmpty();
int __stdcall CountPlayersInAllyGroup(int owner);

void __stdcall ToggleAnyMission(CampaignMissionObject* obj);

#endif
