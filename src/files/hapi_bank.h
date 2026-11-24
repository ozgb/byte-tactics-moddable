#ifndef ACCOUNT_BANK_H
#define ACCOUNT_BANK_H

struct AccountList;
struct StringPool;
struct FileHandle;

class AccountBank {
public:
    AccountList* bank;

    AccountBank* InitBank();
    void CloseBank();
    void NewBank();
    int OpenBank(const char* filename, const char* name, const char* account);
    int SaveBank(const char* filename, const char* name, int compress, int audit);
    void SaveAccount(int index, void* file, StringPool* pool, int compress);
    void LoadAccount(FileHandle* file, int* image, const char* name);
    int OpenAccount(const char* name);
    int SetIntegerItem(const char* name, int value);
    int SetDoubleItem(const char* name, double value);
    int SetStringItem(const char* name, const char* value);
    int GetIntegerItem(const char* name, int def);
    double GetDoubleItem(const char* name, double def);
    const char* GetStringItem(const char* name, const char* def);
    int HasItem(const char* name);
    int FindItem(const char* name, int create);
    int FindNumberedBox(int number, int create);
    int FindNamedBox(const char* name, int create);
    int OpenNumberedBox(int number);
    int OpenNamedBox(const char* name);
    int GetBoxSize();
    void SeekBox(int pos);
    int SeekBoxEnd();
    int ReadBox(void* dst, int len);
    int WriteBox(const void* src, int len);
    void WriteAuditFile(const char* filename);
};

#endif
