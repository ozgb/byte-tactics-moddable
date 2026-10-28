#ifndef CONSOLE_COMMANDS_H
#define CONSOLE_COMMANDS_H
struct FixedWorldPos;
struct Point;

class CommandArgs;

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

void __stdcall CmdMemDump(char* args);
void __stdcall CmdMem(int);
void __stdcall CmdAssert(int);
void __stdcall CmdDPrint(int);
void __stdcall CmdBurnAll(int arg1);
void __stdcall CmdBurnOne(void* args);
void __stdcall CmdShading(int unused);
void __stdcall CmdSelectable(int unused);
void __stdcall CmdZBuffer(int);
void __stdcall CmdAntiAlias(int unused);
void __stdcall CmdShadow(int unused);
void __stdcall CmdDither(int unused);
void __stdcall CmdTShadow(int unused);
void __stdcall CmdFShadow(int unused);
void __stdcall CmdLOSType(int unused);
void __stdcall CmdRCache(int unused);
void __stdcall CmdCDStop(int unused);
void __stdcall CmdSound3D(int unused);
void __stdcall CmdScrollSpeed(void* args);
void __stdcall CmdIFace(void* args);
void __stdcall CmdLOS(int unused);
void __stdcall CmdMapping(int unused);
void __stdcall CmdSelBoxes(int unused);
void __stdcall CmdTreeDeath(int unused);
void __stdcall CmdNoShake(int unused);
void __stdcall CmdDoubleShot(int unused);
void __stdcall CmdHalfShot(int unused);
void __stdcall CmdRadar(int unused);
void __stdcall CmdScreenChat(int unused);
void __stdcall CmdSing(int unused);
void __stdcall CmdClock(int unused);
void __stdcall CmdReloadAIProfiles(int arg1);
void __stdcall CmdBigBrother(int unused);
void __stdcall CmdProfile(void* args);
void __stdcall CmdNowISee(void*);
void __stdcall CmdNetStats(int);
void __stdcall CmdMusicMode(void* args);
void __stdcall DefaultCommandHandler(CommandArgs* args);
void __stdcall CmdShootAll(int unused);

void __stdcall WorldToScreenWithHeight(FixedWorldPos* pos, int* screen_x, int* screen_y);
void __stdcall DrawWorldDeltaLine(void* surface, FixedWorldPos* p, short dx, short dz, int color);
void __stdcall DrawCellContours(void* surface, Point* corners, unsigned char* heights);

#endif
