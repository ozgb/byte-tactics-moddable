#ifndef COMMANDS_H
#define COMMANDS_H
class CommandArgs {
public:
    char* args[0x14];
    char buffer[0x7e];
    char unknown_ce[2];
    int count;

    char* GetArg(int index, char* default_val);
    int GetIntArg(int index, int default_val);
    float GetFloatArg(int index, float default_val);
    CommandArgs* InitArgs();
    void Tokenize(char* text, char* end);
    void ShiftArgs(int n);
    void SubstituteArgs(CommandArgs* other);

    char* GetArgPtr(int index, char* def)
    {
        if (index < 0 || index >= count) return def;
        return args[index];
    }
};

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
int __stdcall ExecuteCommandText(char* text, int len, CommandArgs* vars, int mask);

#endif
