#ifndef UNIT_SYNC_H
#define UNIT_SYNC_H

struct Unit_0046e330;
struct UnitSyncEntry;
struct UnitSyncPacket;
struct Target_0046d530;
struct Source_0046d630;

class UnitSync {
public:
    char unknown_0[0x2c];
    int seqSent;
    int seqCur;
    int seqMax;
    char unknown_38[0x20];
    int direct;
    int pendingPlayerCount;
    int checksumProgress;
    int disabled;

    UnitSync(int param);
    ~UnitSync();
    void ResetEntries();
    void SendSyncPacket(unsigned int* param_1, UnitSyncPacket* param_2, int unused);
    void ReceiveSyncPacket(void* param_1, int param_2);
    void SendSyncMessage(unsigned char arg, int a, int b, int unused);
    void SendSyncMessageTo(Target_0046d530* target, unsigned char arg, int a, int b, int unused);
    void SendEntryTo(Target_0046d530* target, unsigned char arg, Source_0046d630* src, int unused);
    void HandleSyncPacket(UnitSyncPacket* packet, unsigned char player);
    void NotifyEntryChanged(unsigned int param_1);
    void CheckUnitAvailable(unsigned int key, int y);
    void ProcessSync();
    char* GetSyncStatusText();
    int AllPlayersSynced();
    int IsPlayerSynced(int id);
    void ApplyToUnitTypes();
    int PopChangedEntry(UnitSyncEntry* out);
    int GetUnitEntry(Unit_0046e330* unit, UnitSyncEntry* out);
    int ToggleUnitAllowed(Unit_0046e330* unit);
    int DisallowUnit(Unit_0046e330* unit);
    int AllowUnit(Unit_0046e330* unit);
    void SetUnitLimit(Unit_0046e330* unit, int value);
};

void __stdcall ReportIntervalTimer(void);

int __stdcall ReportGameEvent(int msg);
void __fastcall DeleteUnitSync();

void __stdcall FUN_0046c8c0(int);
void __stdcall FUN_0046c8d0(void);
void __stdcall CreateUnitSync(int);
void __stdcall NopChecksumEntryDtor(int);
int* __stdcall CopyDwordRangeOverwrite(int* first, int* last, int* dest);

#endif
