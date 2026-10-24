#ifndef CONTROL_PANEL_H
#define CONTROL_PANEL_H

struct Gadget;
struct Gui;
struct Unit;
struct Vec3;

void __stdcall InitCommands();
void __stdcall CopyMenuEntryName(char* dest, unsigned short index);
int __stdcall MenuEntryNameContains(Gadget* entries, char* text, int index);
void __stdcall SetBuildCountText(Gui* obj, unsigned short index, int n);
void __stdcall RefreshBuildCountTexts(Gui* menu, Unit* unit);
void __stdcall UpdateOnOffButton(Unit* unit);
void __stdcall QueueBuildOrder(char* name, Unit* unit, int count);
void __stdcall SetOrderIntent(unsigned char mode);
void __stdcall RefreshOrderButtons(Unit* unit);
int __stdcall HandleOrdersPanelClick(Gui* menu, Gadget* entries);
void __stdcall SetPrevNextGadgetNames(Unit* unit);
void __stdcall HandleBuildPanelClick(Gui* menu);
void __stdcall DisableUnavailableBuildMenuEntries(Gui* obj);
int __stdcall GetBuildMenuPage(Unit* obj);
void __stdcall BuildEntryGuiName(char* dest, unsigned short index, int n);
Unit* __stdcall GetBuildMenuFocusUnit();
void __stdcall FinishConstruction(Unit* unit, Unit* target);
int __stdcall AddBuildProgress(Unit* builder, Unit* unit, float amount);
void __stdcall ApplyUnfinishedBuildDecay(Unit* unit, int ticks);
int __stdcall AddRepairProgress(Unit* builder, Unit* unit, float f);
void __stdcall StepBuildMenuPageBack(int toggle);
void __stdcall OpenBuildMenuPage(int page);
void __stdcall MarkSelectionOrdersDirty(Unit* unit);
void __stdcall DispatchOrdersPanelPageFlags();
void __stdcall ResetCameraState();
void __stdcall CycleCameraFollow(int param);
void __stdcall FindLocalCommander();
void __stdcall ClampCameraPosition();
void __stdcall ClampCameraTarget();
void __stdcall SetCameraPosition(int x, int y, int instant);
void __stdcall StartScreenShake(int dx, int dy, int value);
void __stdcall AccumulateScreenShake(int dx, int dy, int value);
void __stdcall UpdateScreenShake();
void __stdcall CenterCameraOnPoint(int a, int b, int c);
void __stdcall CenterCameraOnMapPosition(Vec3* p, int);
void __stdcall UpdateCameraFollow();
void __stdcall BeginMouseScroll();
void __stdcall EndMouseScroll();
void __stdcall UpdateMouseScroll();
void __stdcall UpdateEdgeScroll();

#endif
