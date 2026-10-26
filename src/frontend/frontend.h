#ifndef FRONTEND_H
#define FRONTEND_H

void __stdcall CheckFrontendStateChange(int line, char* file);
void __stdcall SetFrontendErrorText(char*);
void __stdcall ShowFrontendErrorText();
void __stdcall FlushKeyQueue(void);
void __stdcall SetFrontendSubState(char state, int line, char* file);
void __stdcall ApplyPendingSubState();
void __stdcall ResetFrontendState();
void __stdcall EnterMainMenuState();
void __stdcall PresentFrontendFrame();
void __stdcall UpdateMenuSparks();
void __stdcall OpenCloseCdPlayerDialog();
void __stdcall OpenMainMenu();
void __stdcall PlayMovie(char*);
void __stdcall LoadWarpLevel(int);
void __stdcall HandleFrontendDebugKey(void);
int __stdcall ConnectToService(void);
void __stdcall RunFrontendStateMachine(void);
void __stdcall ClearPictureCache();
int* __stdcall FindCachedPicturePalette(const char* name);
int __stdcall FindCachedPicture(char* name);
void __stdcall OrLabelAttribs(void);
void __stdcall EmptyPreFrontendInitHook(void);
void __stdcall EmptyPostSimStepHook(void);
void __stdcall EmptyPostSimStepHook_B(void);
void __stdcall EmptyPostSimStepHook_C(void);
void __stdcall EmptyMainLoopHook(void);
void __stdcall EmptyMainLoopHook_B(void);
void __stdcall EmptyMainLoopHook_C(void);

int __stdcall CodeChecksumFailed(void);
void __stdcall BlankScreen();
void __stdcall FreePictureCache();
int __stdcall MainLoopContinueStub(void);
int __stdcall MainLoopContinueStub_B(void);
int __stdcall MainLoopContinueStub_C(void);

#endif
