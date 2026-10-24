#ifndef COMMANDS_H
#define COMMANDS_H

void __stdcall ClearDefaultCommandHandler();
int __stdcall FindLineEnd(char* text, int length);
void __stdcall NopRet(void);
void __stdcall ClearCommandTable();

#endif
