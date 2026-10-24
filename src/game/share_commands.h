#ifndef SHARE_COMMANDS_H
#define SHARE_COMMANDS_H

class CommandArgs;

void __stdcall ToggleShareLos(int unused);
void __stdcall CmdSetShareMetal(CommandArgs* args);
void __stdcall CmdSetShareEnergy(CommandArgs* args);

void __stdcall CmdShareMetal(int unused);
void __stdcall CmdShareEnergy(int unused);
void __stdcall CmdShareMapping(int unused);
void __stdcall CmdShareRadar(int unused);
void __stdcall CmdShareAll(int unused);
void __stdcall CmdShowRanges(int unused);
void __stdcall CmdCompression(int unused);
void __stdcall CmdBPS(int unused);
void __stdcall CmdSFX(int arg1);

#endif
