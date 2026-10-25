#pragma once
#include <windows.h>
#include <stdio.h>
#define GetNativeSystemInfo GetSystemInfo
#define snprintf _snprintf
#ifndef FILE_MAP_EXECUTE
#define FILE_MAP_EXECUTE 0x20
#endif
#define ComputerNameDnsFullyQualified 0
static BOOL GetComputerNameExW(int t, LPWSTR b, LPDWORD n) { return FALSE; }
