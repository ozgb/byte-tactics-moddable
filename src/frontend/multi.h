#ifndef MULTI_H
#define MULTI_H

#include "../../include/win_types.h"
struct Gui;

struct Gadget;
struct Layer;
struct Player;

void __stdcall UpdateGameSelection(Gui* menu, Gadget* entry);
int __stdcall ConnectToGame(Layer* gadget);
void __stdcall HandleNewMultiClick(Gui* gadget);
void __stdcall OpenNewMultiDialog();
int __cdecl GetLocalPlayerPassword();
int __stdcall GetServiceProviderIndex();
int __stdcall BuildCompoundAddress(int* addressOut, int* sizeOut);
int __stdcall TryConnect(int unused);
void __stdcall HandleTcpDialogClick(Gui* gadget);
void __stdcall OpenTcpDialog();
void __stdcall SetSerialBaudFromGadget(Gui* menu, Gadget* entry);
void __stdcall SetSerialPortFromGadget(Gui* menu, Gadget* entry);
void __stdcall HandleSerialDialogClick(Gui* gadget);
void __stdcall OpenSerialDialog();
void __stdcall FillAccountList(void);
void __stdcall RefreshAccountList(void);
void __stdcall ShowSelectedAccount(Gui* menu, Gadget* player);
void __stdcall SaveModemNumbers(void);
void __stdcall HandleModemDialogClick(Gui* gadget);
BOOL __stdcall EnumModemAddressCallback(const GUID& guidDataType, DWORD dataSize, LPCVOID data, LPVOID context);
void __stdcall OpenModemDialog();
void __stdcall HandleReportClick(Gui* obj);
void __stdcall ReportDialogFrame(void);
void __stdcall OpenReportDialog(unsigned int* count, char** names);
int __stdcall InitScoreReporting(void);
void __stdcall HandleSelectGameClick(Gui*);
void __stdcall OpenSelectGameDialog();
int __stdcall CloneServiceSlot(Gadget* entries, int, short, int, char*);
void __stdcall CacheLogosGadgetIndex(Gui* param1, Gadget* param2);
void __stdcall HandleLogoSelectClick(Gui*);
void __stdcall ShowSelectedMapInfo();
void __stdcall HandleViewMapClick(Gui*);
void __stdcall OpenViewMapDialog();
void __stdcall UpdateMapSelection(Gui* menu, Gadget* unused);
void __stdcall HandleMapSelectClick(Gui*);
void __stdcall OpenMultiMapSelector();
void __stdcall OpenLogoSelectDialog();
void __stdcall ExpandGadgetTextToType5(Gadget*);
void __stdcall SwapPlayerSlots(Player*, Player*);
void __stdcall CompactActivePlayerSlots();
void __stdcall UpdateBattleRoomFlags();
void __stdcall HandleRejectChoice(Gui* gadget);
void __stdcall OpenRejectDialog(int player);
void __stdcall HandleDisplayModesClick(Gui* gui);
void __stdcall CyclePlayerDisplayMode(void);
void __stdcall UpdateWatchingGadgets();
void __stdcall HandleControlDialogClick(Gui* gui);
void __stdcall OpenControlDialog();
bool __stdcall ArePlayersAllied(Player* a, Player* b);
int __stdcall CountPlayersInAlliance(int alliance);
char __stdcall AreAllPlayersInOneAlliance();
int __stdcall FindNextAlly(int player, int start);
void __stdcall RefreshTeamIcons();
void __stdcall SyncMutualAlliances();
void __stdcall ClearAlliances(Player* player);
void __stdcall CyclePlayerAlliance(int index);
void __stdcall RebuildAllyList();
void __stdcall HandleAlliesClick(Gui* gadget);
void __stdcall RefreshAlliesScreen(int);
void __stdcall OpenAlliesDialog();
void __stdcall UpdateSideGadget(int side);
void __stdcall RefreshBattleRoomRows();
void __stdcall HandleEndMultiClick(Gui* obj);
void __stdcall OpenEndMultiScreen();
void __stdcall FreeSaveListBuffers();
void __stdcall LoadUnitRestrictListFile(char* name);
void __stdcall SaveUnitRestrictListFile(char* filename);
void __stdcall CopySelectedGameName();
void __stdcall HandleLoadListClick(Gui* menu);
void* __stdcall ListSaveGameFiles(int* out);
void __stdcall ShowSelectedSaveGame(Gui* unused1, Gadget* unused2);
void __stdcall HandleSaveGameClick(Gui* menu);
void __stdcall OpenSaveGameDialog();
void __stdcall OpenLoadListDialog();
void __stdcall HandleUnitCountSlider(void* obj, char* gadget);
void __stdcall LoadUnitPortrait();
void __stdcall UnitRestrictDialogFrame();
void __stdcall ShowSelectedUnitCosts(Gui* panel, Gadget* unit);
int __cdecl CompareUnitRestrictEntries(const char* a, const char* b);
void __stdcall OpenUnitRestrictions();

void __cdecl BuildPlayerSlotGadgets(void);
int __stdcall SelectConnection(int index);
void __stdcall HandleSelectProviderClick(Gui* menu);
void __stdcall FillProviderList();
void __stdcall OpenBattleRoom();
void __stdcall UpdateBattleRoom();
void __stdcall HandleBattleRoomClick(Gui* gadget);
void __stdcall UpdateUnitSliders(Gui*, int unused);

void __stdcall UpdateMaxUnitsText(Gui* gui, int index);
void __stdcall UpdateMetalText(Gui* sub, int unused);
void __stdcall UpdateEnergyText(Gui* sub, int unused);
void __stdcall SetNamedSliderValue(Gui* menu, char* name, int value);
void __stdcall HandleRestrictionsClick(Gui* menu);

#endif
