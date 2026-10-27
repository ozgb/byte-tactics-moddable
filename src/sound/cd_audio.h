#ifndef CD_AUDIO_H
#define CD_AUDIO_H

struct Pos_004cf570 {
    int x, y, z;
};

void __stdcall NopRet4(int);
void __cdecl HandleCdMessage(int msg, int wParam, int lParam);
int __stdcall GetTrackLength(int track);
void __stdcall OnNextTrackTimer(void*);
void __stdcall OnCdFadeTimer(void*);
int __stdcall GetCdPosition();
void __stdcall NopRet4_B(int);

#endif
