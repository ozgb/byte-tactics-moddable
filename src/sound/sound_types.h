#ifndef SOUND_TYPES_H
#define SOUND_TYPES_H
class TdfFile;

void __stdcall LoadSoundCategories();
void __stdcall FreeSoundCategories();
void __stdcall LoadAllSound();
void __stdcall FreeSounds();
void __stdcall FUN_0042f900(int);

int __stdcall ReadSoundEntry(TdfFile* source, char* key, int* out);

#endif
