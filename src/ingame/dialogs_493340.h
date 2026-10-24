#ifndef DIALOGS_493340_H
#define DIALOGS_493340_H

struct Gadget;
struct Gui;

void __stdcall UpdateMetalReadout(Gui* obj, int unused);
void __stdcall UpdateEnergyReadout(Gui* obj, int unused);
void __stdcall GiveSelectedUnitsToPlayer(unsigned char player);
void __stdcall HandleShareDialogEvent(Gui* obj);
void __stdcall OpenShareDialog();
void __stdcall ResetPlayerGadgets(void);
void __stdcall HandleTalkDialogEvent(Gui* gadget);
void __stdcall OpenTalkDialog();
void __stdcall HandleUnitInfoDialogEvent(Gui* gadget);
void __stdcall DrawUnitInfoImage(Gui* gadget, Gadget* entry);
void __stdcall OpenUnitInfoDialog(void);
void __stdcall HandleTabMenuEvent(Gui* gadget);
void __stdcall HandleTabMenuCancel(Gui* gadget);
void __stdcall FlashScorePanelKillLoss(int, int);
void __stdcall UpdateCdCategoryByActivity();
void __stdcall AddCdActivitySample(int);
void __stdcall ToggleTabMenu();
void __stdcall OpenMain2Layout();
void __stdcall UpdateFramePacing();
void __stdcall RunGameSteps(int showStats);
void __stdcall HandleDebugHotkey(int eventType);

#endif
