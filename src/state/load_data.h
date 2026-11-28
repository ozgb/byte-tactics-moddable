#ifndef STATE_LOAD_DATA_H
#define STATE_LOAD_DATA_H

void __cdecl LoadDataAttach(unsigned char* native, unsigned base, unsigned size, void (__stdcall* fatal)(const char* text));

void __cdecl LoadDataRegister(const void* buffer, unsigned size);

void __cdecl LoadDataRegisterTable(const void* table, unsigned size);

void __cdecl LoadDataRegisterFrame(const void* frame, unsigned size);

void __cdecl LoadDataUnregister(const void* buffer);

void __cdecl LoadDataStoreByte(unsigned char* native, unsigned char value);

unsigned __cdecl LoadAddress(const void* native);

unsigned __cdecl LoadAddressIfLoadData(const void* native);

void* __cdecl LoadNative(unsigned address);

void* __cdecl LoadNativeIfLoadData(unsigned address);

void __cdecl LoadDataCheck(unsigned tick);

unsigned __stdcall LoadGetGafFrame(unsigned table, int index);

void* __stdcall LoadGetGafFrameNative(unsigned table, int index);

void* __stdcall LoadGetGafSequenceFrame(void* cursor);

void __stdcall LoadInitGafSequence(void* cursor, unsigned animation, int index);

int __stdcall LoadStepGafSequence(void* cursor);

#endif
