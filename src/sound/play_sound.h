#ifndef PLAY_SOUND_H
#define PLAY_SOUND_H

int __stdcall IsWindowsSoundAvailable(void);
void __stdcall ResumeLoopingWav(void);
void __stdcall StopWindowsSound();
int __stdcall PlayWavFromDisk(char* path);

#endif
