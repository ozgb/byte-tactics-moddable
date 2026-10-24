#ifndef CONSOLE_COMMANDS_H
#define CONSOLE_COMMANDS_H

class CommandArgs;
struct Point16;

void __stdcall CmdAI(CommandArgs* args);
void __stdcall CmdAssign(CommandArgs* args);
void __stdcall CmdFeature(CommandArgs* args);
void __stdcall CmdKill(CommandArgs* args);
void __stdcall CmdSwitchAlt(CommandArgs* args);
void __stdcall CmdLight(CommandArgs* args);
void __stdcall CmdEdge(CommandArgs* args);
void __stdcall CmdSearch(CommandArgs* args);
void __stdcall CmdCDPlay(CommandArgs* args);
void __stdcall CmdMove(CommandArgs* args);
void __stdcall CmdLogo(CommandArgs* args);
void __stdcall CmdIWin(CommandArgs* args);
void __stdcall CmdILose(CommandArgs* args);
void __stdcall CmdSeaLevel(CommandArgs* args);
void __stdcall CmdControl(CommandArgs* args);
void __stdcall CmdView(CommandArgs* args);
void __stdcall CmdGive(CommandArgs* args);
void __stdcall CmdContour(CommandArgs* args);
void __stdcall CmdNow(CommandArgs* args);
void __stdcall CmdATM(CommandArgs* args);
void __stdcall CmdNoMetal(CommandArgs* args);
void __stdcall CmdNoEnergy(CommandArgs* args);
void __stdcall CmdGamma(CommandArgs* args);
void __stdcall CmdFilm(CommandArgs* args);
void __stdcall CmdFilmSpeed(CommandArgs* args);
void __stdcall CmdSave(CommandArgs* args);
void __stdcall CmdReload(CommandArgs* args);
void __stdcall CmdMakePoster(CommandArgs* args);
void __stdcall CmdMeteor(CommandArgs* args);
void __stdcall CmdDrop(CommandArgs* args);
void __stdcall CmdInclude(CommandArgs* args);
void __stdcall CmdDebugBreak(CommandArgs* args);
void __stdcall WorldToScreenTileOffset(int x, int z, int* screen_x, int* screen_y);
void __stdcall DrawWorldSegmentLine(void* surface, Point16 from, short dx, short dz, int color);
void __stdcall CmdPrintWeights(CommandArgs* args);
void __stdcall CmdSenderror(CommandArgs* args);

#endif
