#ifndef WIN32_SETTINGS_H
#define WIN32_SETTINGS_H

#include <windows.h>
#include "../../../include/win_types.h"

int __stdcall ReadRegistryValue(const char* app, const char* key, void* buf, unsigned int* size);
int __stdcall AccessRegistryValue(char* subKey, char* valueName, LPBYTE data, LPDWORD size, DWORD type, DWORD read);
int __stdcall ReadRegistryData(void* subKey, void* valueName, void* data, void* size);
int __stdcall ReadRegistryDword(void* subKey, void* valueName, void* value);
void __stdcall WriteRegistryBinary(void* subKey, void* valueName, void* data, int unused);
void __stdcall WriteRegistryString(void* subKey, void* valueName, char* value);
void __stdcall WriteRegistryDword(char* key, char* name, int value);

#endif
