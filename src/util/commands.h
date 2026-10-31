#ifndef COMMANDS_H
#define COMMANDS_H
#include "../../interfaces/services.h"

struct ConsoleCommand {
    const char* name;
    void (__stdcall* run)(CommandArgs* args);
    int mask;
};

void __stdcall ClearDefaultCommandHandler();
int __stdcall FindLineEnd(char* text, int length);
void __stdcall NopRet(void);
void __stdcall ClearCommandTable();

void __stdcall RegisterCommands(ConsoleCommand* rec);
int __stdcall ExecuteCommand(CommandArgs* obj, int mask);

#endif
