#ifndef SHARE_COMMANDS_H
#define SHARE_COMMANDS_H

class CommandArgs;

void __stdcall ToggleShareLos(int unused);
void __stdcall CmdSetShareMetal(CommandArgs* args);
void __stdcall CmdSetShareEnergy(CommandArgs* args);

#endif
