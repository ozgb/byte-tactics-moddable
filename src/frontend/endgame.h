#ifndef ENDGAME_H
#define ENDGAME_H

struct Gui;

struct Data {
    char unknown_0[0x14];
    char* items;
    char unknown_18[8];
};

void __stdcall CreateGameObject();
void __stdcall SetMissionStatus(int param1, int param2, int param3);
void __stdcall SetUpEndMissionScreen();
void __stdcall StartScreenFade(void);
void __stdcall StepScreenFade(void);
void __stdcall StartPaletteFade(unsigned char* target, unsigned char* current, int steps);
void __stdcall ScheduleFadeTick(void);
int __stdcall IsFadeDone();
void __stdcall StepPaletteFade(void);
void __stdcall FillEndGameStatistics(void);
void __stdcall CollectEndGameStats();
int __stdcall AreStatBarsComplete(void);
char* __stdcall BuildScrollItems1(char* names, char* flags, int count);
char* __stdcall BuildScrollItems2(char* names, int a, int b);
void __stdcall HandleEndMissionClick(Gui* gadget);
int __stdcall ShouldShowNextMission();
void __stdcall OpenEndMissionScreen();
void __stdcall EnableEndMissionButtons();
void __stdcall ActivatePlayerGadgets(char* prefix);
void __stdcall ShowEndMissionScreen();
void __stdcall HandleCdCheckClick(Gui* gadget);
void __stdcall OpenCdCheckDialog();
int __stdcall DrawEndGameFrame();
void __stdcall RunEndGameState();
void __stdcall InitMemoryCache(void);

void __stdcall SetEndGameState(unsigned int state);
void __stdcall InitMissionStatus(void);
void __cdecl FreeMemoryCache();

#endif
