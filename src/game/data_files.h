#ifndef DATA_FILES_H
#define DATA_FILES_H

void __stdcall CheckGpfVersion();
void* __stdcall LoadFontByName(const char* name);
void* __stdcall LoadPaletteByName(char* name);
void* __stdcall LoadAnimGaf(char* name);
void __stdcall FindGafSequence(int gaf, char* name);
void __stdcall LoadGameResources();
void __stdcall FreeAnimFiles();
void __stdcall AddAnimplayPointer(void* item);
void* __stdcall LoadObject3d(const char* name);
void __stdcall LoadGameFonts();
void __stdcall FreeGameFonts();
void __stdcall LoadTextureGafs();
void __stdcall FreeTextureGafs();

#endif
