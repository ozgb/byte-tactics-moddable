#pragma once
#define GetNativeSystemInfo GetSystemInfo
#define snprintf _snprintf
#ifndef FILE_MAP_EXECUTE
#define FILE_MAP_EXECUTE 0x20
#endif
#define ComputerNameDnsFullyQualified 0
static int GetComputerNameExW(int t, unsigned short* b, unsigned long* n) { return 0; }
