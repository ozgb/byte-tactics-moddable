#ifndef STATE_LOAD_DATA_H
#define STATE_LOAD_DATA_H

// Makes the load data region of the modules' shared memory live: size bytes at native, whose linear address is base; fatal is called with the text of a failure.
void __cdecl LoadDataAttach(unsigned char* native, unsigned base, unsigned size, void (__stdcall* fatal)(const char* text));

// Marks a loaded GAF buffer of size bytes as load data a block field may point into; it is copied into the region on first use.
void __cdecl LoadDataRegister(const void* buffer, unsigned size);

// Marks a frame table the engine made (a count, then 8-byte entries from offset 0x28, each a frame of LoadDataRegisterFrame) as load data.
void __cdecl LoadDataRegisterTable(const void* table, unsigned size);

// Marks a frame the engine made (a GafFrame header followed by its pixels) as load data.
void __cdecl LoadDataRegisterFrame(const void* frame, unsigned size);

// Forgets a registered buffer, table or frame before it is freed; its copy stays in the region until none is left in use.
void __cdecl LoadDataUnregister(const void* buffer);

// Stores a byte into registered load data, in its copy as well when it is mapped already.
void __cdecl LoadDataStoreByte(unsigned char* native, unsigned char value);

// The load address of a native pointer into registered load data (0 for null): its linear address once the region is attached, else the pointer itself.
unsigned __cdecl LoadAddress(const void* native);

// LoadAddress for a pointer that may be registered load data, else the pointer itself.
unsigned __cdecl LoadAddressIfLoadData(const void* native);

// The native pointer of a load address (null for 0), in the original buffer; once the region is attached, any other value is fatal.
void* __cdecl LoadNative(unsigned address);

// LoadNative for a harness value that may be a load address or a native pointer: a native pointer, or an address outside the mapped copies, comes back unchanged.
void* __cdecl LoadNativeIfLoadData(unsigned address);

// Ends the game when a module changed the mapped load data since it was mapped.
void __cdecl LoadDataCheck(unsigned tick);

// GetGafFrame of a frame table by its load address, giving the frame's load address.
unsigned __stdcall LoadGetGafFrame(unsigned table, int index);

// GetGafFrame of a frame table by its load address, giving the native frame.
void* __stdcall LoadGetGafFrameNative(unsigned table, int index);

// GetGafSequenceFrame of a cursor of the block whose animation is a load address, giving the native frame.
void* __stdcall LoadGetGafSequenceFrame(void* cursor);

// InitGafSequence of a cursor of the block on an animation by its load address; the cursor keeps the load address.
void __stdcall LoadInitGafSequence(void* cursor, unsigned animation, int index);

// StepGafSequence of a cursor of the block whose animation is a load address.
int __stdcall LoadStepGafSequence(void* cursor);

#endif
