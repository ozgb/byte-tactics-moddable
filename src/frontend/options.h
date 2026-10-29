#ifndef OPTIONS_H
#define OPTIONS_H
struct Gadget;
struct ValueTextEntry;

struct Gadget;
struct Gui;

void __stdcall SetDirectConnectAddress(char*);
void __stdcall SetHostGameName(int flag, char* text);
void __stdcall SetDirectPlayProvider(int);
void __stdcall SetGadgetsDisabledByPrefix(char* name, int value);
void __stdcall DeactivateGadgetsByPrefix(char* prefix);
void __stdcall SetGadgetStatusAndText(int index, int state, char* offText, char* onText);
void __stdcall ClearGadgetText(void*, int);
void __stdcall ApplyBrightnessAndVolume();
void __stdcall SetGameSpeedLabel(char* name, char* label, int speed, int normal);
void __stdcall SetLockedCdTrack(int value);
void __stdcall UpdateTrackGadgets();
void __stdcall ApplyTrackType();
void __stdcall ApplyDefaultSoundOptions();
void __stdcall ApplyDefaultMusicOptions();
void __stdcall ApplyDefaultUiOptions();
void __stdcall ApplyDefaultVisualOptions();
void __stdcall LoadSavedAudioSettings();
void __stdcall RestoreSoundOptions();
void __stdcall RestoreUiOptions();
void __stdcall RestoreVisualOptions();
void __stdcall LoadSavedSettings();
void __stdcall SaveGameSettings();
void __stdcall EnsureOptionsPanelGadget();
void __stdcall BlitOptionsPanel();
int __cdecl OpenOptionsLayout();
void __stdcall TickMusicOptions();
void __stdcall UpdateMusicGadgets();
void __stdcall OpenMusicOptions();
void __stdcall UpdateSoundGadgets();
void __stdcall OpenSoundOptions();
void __stdcall OpenVisualOptions(int);
void __stdcall OpenSpeedOptions();
void __stdcall ShowGameSettingsDialog();
void __stdcall OpenBriefingDialog();
void __stdcall OpenHelpDialog();
void __stdcall ClearScreenWithHudPalette();
void __stdcall CloseOptionsPanel();
void __stdcall OpenOptionsPanel();
void __stdcall OpenRestartDialog();
void __stdcall OpenSurrenderDialog();
void __stdcall OpenExitMenu();
void __cdecl HandleBattleQuitPrompt(int);
void __stdcall OpenInGameOptions();

void __stdcall SetSliderFromValue(Gadget*, int value);
int __stdcall ReadSliderValue(Gadget*);
void __stdcall HandleVisualOptionsClick(Gui* gui);
void __stdcall HandleGameSettingsDialogClick(Gui* gui);
void __stdcall HandleBriefingClick(Gui* gui);
void __stdcall FillHelpPage(Gui* sub, int page, int lineCount);
void __stdcall HandleHelpClick(Gui* gui);
void __stdcall HandleOptionsPanelClick(Gui* gui);
void __stdcall HandleRestartDialogClick(Gui* gui);
void __stdcall HandleSurrenderChoice(Gui* gui);
void __stdcall HandleExitMenuClick(Gui* gui);
void __stdcall HandleInGameOptionsClick(Gui* gui);

void __stdcall UpdateVideoModeLabel(Gui*, Gadget*);
void __stdcall HandleVideoModeSlider(Gui* obj, int unused);
void __stdcall HandleGammaSlider(Gui* obj, int unused);
void __stdcall HandleEffectsVolumeSlider(Gui* obj, int unused);
void __stdcall HandleMusicVolumeSlider(Gui* obj, int unused);
void __stdcall SetGadgetTextFromValueTable(ValueTextEntry* table, char* name, int value);
void __stdcall HandleGameSpeedSlider(Gui* obj, int unused);
void __stdcall HandleScreenSlider(Gui* obj, int unused);
void __stdcall HandleMaxLinesSlider(Gui* obj, int unused);
void __stdcall HandleTextScrollSlider(Gui* obj, int unused);

void __stdcall DrawOptionsScrollBar(void* surf);

#endif
