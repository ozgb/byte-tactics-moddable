#ifndef DATA_FILES_H
#define DATA_FILES_H
struct GafFrame;

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

char* __stdcall BuildDataPath(char* buf, const char* dir, const char* name, const char* ext);
int __stdcall LoadSoundByName(char* name, const char* file);
GafFrame* __stdcall LoadRadarPic(char* path, int* outX, int* outY);
char* __stdcall LoadFileWithProgress(char* path);
int __stdcall LoadDefaultPalette();

#endif
