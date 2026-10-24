#ifndef SAVEGAME_H
#define SAVEGAME_H

struct Gui;

void __stdcall ShowSavedGameInfo();
void __stdcall ShowInvalidSavegameDialog(void*);
void __stdcall LoadGameScreenHandler(Gui* gadget);
char* __stdcall ListSavedGames(int* count);
void __stdcall SavedGameSelectHandler(int arg1, int arg2);
void __stdcall SaveGameScreenHandler(Gui* gadget);
void __stdcall ShowSaveGameScreen();
void __stdcall ShowLoadGameScreen();

#endif
