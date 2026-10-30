#ifndef SIDE_DATA_H
#define SIDE_DATA_H

class TdfFile;

#pragma pack(push, 1)
struct SideData {
    char unknown_0[0x95];
    unsigned char bSideId;
};
#pragma pack(pop)

void __stdcall LoadLogos();
void __stdcall FreeLogos(void);
void __stdcall ReadSideRect(TdfFile* obj, int* out, char* name, char* side);
void __stdcall FreeSideFonts();
void __stdcall LoadSideData(void);

#endif
