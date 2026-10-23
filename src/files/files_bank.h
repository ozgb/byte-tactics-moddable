#ifndef FILES_BANK_H
#define FILES_BANK_H

#include "../../interfaces/seams.h"

struct AccountList;

class HapiBank {
public:
    AccountList* bank;

    HapiBank* InitBank()
    {
        return g_seams.files->BankInit(this);
    }
    void CloseBank()
    {
        g_seams.files->BankClose(this);
    }
    void NewBank()
    {
        g_seams.files->BankNew(this);
    }
    int OpenBank(char* filename, char* name, char* account)
    {
        return g_seams.files->BankOpen(this, filename, name, account);
    }
    int SaveBank(char* filename, char* name, int compress, int audit)
    {
        return g_seams.files->BankSave(this, filename, name, compress, audit);
    }
    int OpenAccount(char* name)
    {
        return g_seams.files->BankOpenAccount(this, name);
    }
    int SetIntegerItem(const char* name, int value)
    {
        return g_seams.files->BankSetInteger(this, name, value);
    }
    int SetDoubleItem(const char* name, double value)
    {
        return g_seams.files->BankSetDouble(this, name, value);
    }
    int SetStringItem(const char* name, char* value)
    {
        return g_seams.files->BankSetString(this, name, value);
    }
    int GetIntegerItem(char* name, int def)
    {
        return g_seams.files->BankGetInteger(this, name, def);
    }
    double GetDoubleItem(char* name, double def)
    {
        return g_seams.files->BankGetDouble(this, name, def);
    }
    char* GetStringItem(char* name, char* def)
    {
        return g_seams.files->BankGetString(this, name, def);
    }
    int HasItem(const char* name)
    {
        return g_seams.files->BankHasItem(this, name);
    }
    int OpenNumberedBox(int number)
    {
        return g_seams.files->BankOpenNumberedBox(this, number);
    }
    int OpenNamedBox(char* name)
    {
        return g_seams.files->BankOpenNamedBox(this, name);
    }
    int GetBoxSize()
    {
        return g_seams.files->BankGetBoxSize(this);
    }
    void SeekBox(int pos)
    {
        g_seams.files->BankSeekBox(this, pos);
    }
    int ReadBox(void* dst, int len)
    {
        return g_seams.files->BankReadBox(this, dst, len);
    }
    int WriteBox(void* src, int len)
    {
        return g_seams.files->BankWriteBox(this, src, len);
    }
};

#endif
