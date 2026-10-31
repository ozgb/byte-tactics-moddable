#ifndef SKIRMISH_MENU_H
#define SKIRMISH_MENU_H
struct Player;

struct Gui;
struct Rec1;

void __stdcall RefreshAllyIcons(void);
int __stdcall AreAllPlayersInOneAllyGroup(void);
void __stdcall CycleSlotController(int playerIndex);
void __stdcall BindGadgetAnimSequence(Rec1* obj, char* name);
void __stdcall BuildSkirmishPlayerRows(void);
void __stdcall RefreshSkirmishSetup();
int __stdcall FindNextAllySlot(int self, int start);
void __stdcall ApplySlotsToGamePlayers();
void __cdecl CycleCurrentPlayerSide();
void __stdcall HandleSkirmishMapClick(Gui* menu);
void __stdcall UpdateSkirmishMapSelection(Gui* menu, Gadget* unused);
void __stdcall OpenSkirmishMapSelector();
void __stdcall SelectLogosGadget(Gui* obj, void* unused);
void __stdcall CyclePlayerColor(int reverse);
void __cdecl CycleCurrentPlayerAllyGroup();
void __stdcall HandleSkirmishClick(Gui* menu);
void __stdcall HandleSkirmishCheatText(Gui* menu);
void __stdcall OpenSkirmishMenu(void);

void __stdcall AnnounceForcesDestroyed(Player* player);
void __stdcall AnnouncePlayerTookLead(const char* name, int kills);

#endif
