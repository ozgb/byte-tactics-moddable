#ifndef COMMANDS_H
#define COMMANDS_H
class CommandArgs;
struct ConsoleCommand;

void __stdcall ClearDefaultCommandHandler();
int __stdcall FindLineEnd(char* text, int length);
void __stdcall NopRet(void);
void __stdcall ClearCommandTable();

void __stdcall RegisterCommands(ConsoleCommand* rec);
int __stdcall ExecuteCommand(CommandArgs* obj, int mask);
int __stdcall ExecuteCommandText(char* text, int len, CommandArgs* vars, int mask);

#endif
