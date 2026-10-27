#ifndef UNIT_SYNC_H
#define UNIT_SYNC_H

struct Unit_0046e330;
struct UnitSyncEntry;
struct UnitSyncPacket;
struct Target_0046d530;
struct Source_0046d630;

class UnitSync;

void __stdcall ReportIntervalTimer(void);

int __stdcall ReportGameEvent(int msg);
void __fastcall DeleteUnitSync();

void __stdcall FUN_0046c8c0(int);
void __stdcall FUN_0046c8d0(void);
void __stdcall CreateUnitSync(int);
void __stdcall NopChecksumEntryDtor(int);
int* __stdcall CopyDwordRangeOverwrite(int* first, int* last, int* dest);

#endif
