#ifndef SOUND_TYPES_H
#define SOUND_TYPES_H
struct TdfFileCursorView;

void __stdcall LoadSoundCategories();
void __stdcall FreeSoundCategories();
void __stdcall LoadAllSound();
void __stdcall FreeSounds();
void __stdcall FUN_0042f900(int);

int __stdcall ReadSoundEntry(TdfFileCursorView* source, char* key, int* out);

#endif
