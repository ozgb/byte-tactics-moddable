#ifndef PACKETS_H
#define PACKETS_H

int __stdcall InitPacketManager(int bufferCount, int packetCount);
void __cdecl PacketTrace(const char* fmt, ...);
int __stdcall SendToDPID(int from, int to, void* data, int size);
void __stdcall NopRetC_B(int, int, int);
int __fastcall GetCurrentBuffer(int * ecx);
void __stdcall RegisterEmptyAtexit();
void __stdcall EmptyAtexitHandler(void);
void __stdcall ResetChatHudIndices();

#endif
