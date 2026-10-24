#ifndef GAME_SETTINGS_H
#define GAME_SETTINGS_H

void __stdcall SaveTrackSettings(unsigned char* tracks);
void __stdcall LoadSettings();
void __stdcall ApplyUseOnlyUnits();

void __stdcall WriteGameRegistryValue(void* key, void* buf, int value);
int __stdcall ReadGameRegistryValue(const char* key, void* buf, unsigned int* size);
void __stdcall SaveSettings();

#endif
