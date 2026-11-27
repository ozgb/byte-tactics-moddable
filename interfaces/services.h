#ifndef INTERFACES_SERVICES_H
#define INTERFACES_SERVICES_H

#include "rand_stream.h"
#include "sim_events.h"

// Major version of the services interface, changed when a declaration is removed or its signature changes.
#define SERVICES_VERSION_MAJOR 4
// Minor version of the services interface, changed when a declaration is added.
#define SERVICES_VERSION_MINOR 2

// Advances the named random stream of the game logic (LIBCMT rand) and returns the next value, 0 to 0x7fff; the stream's state is in the block.
int __cdecl GameRand(RandStream stream);
// Returns a random number in [0, range) from a generator of its own (rng.seed in the recordings), not a game logic stream; a range below 2 gives 0 and leaves the generator alone.
int __stdcall RandomInt(int range);
// Shows the message in the window of the display and ends the program.
void __stdcall FatalError(const char* message);
// Builds dir/name.ext below the data directory in buf and returns buf.
char* __stdcall BuildDataPath(char* buf, const char* dir, const char* name, const char* ext);
// The extension of TDF data files.
extern char g_extTdf[4];

// A record of a parsed TDF file, which a module holds only as a handle.
class TdfRecord;
// A parsed TDF file as a module holds it in its own memory: TdfFile_Construct fills it, and its records are handles.
struct TdfFileLayout {
    // The root record, the whole file.
    TdfRecord* root;
    // The current record, which TdfFile_SelectRecord selects and the TdfRecord services read.
    TdfRecord* current;
    // Nonzero when the file was read from an archive.
    int flags;
};
// Builds an empty TDF file in place.
void __stdcall TdfFile_Construct(TdfFileLayout* file);
// Unloads a TDF file and frees the engine's records.
void __stdcall TdfFile_Destruct(TdfFileLayout* file);
// Loads and parses the TDF file at a path below the data directory; returns nonzero when it was read.
int __stdcall TdfFile_LoadFile(TdfFileLayout* file, const char* path);
// Selects the child of the given name of the current record (of the root when none is current) as the current one; returns nonzero when there is one, and leaves no record current when there is none.
int __stdcall TdfFile_SelectRecord(TdfFileLayout* file, const char* name);
// Reads an integer field of a record, or def when it is missing.
int __stdcall TdfRecord_GetFieldInt(TdfRecord* record, const char* name, int def);
// Reads a double field of a record, or def when it is missing.
double __stdcall TdfRecord_GetFieldDouble(TdfRecord* record, const char* name, double def);
// Copies a field of a record into dst of size bytes, cut to fit and terminated, and returns 1; when it is missing, copies def whole (unless null) and returns 0.
int __stdcall TdfRecord_GetFieldString(TdfRecord* record, char* dst, const char* key, int size, const char* def);

// A saved-game bank, which a module holds only as a handle.
class HapiBank;
// Opens the named account of a saved-game bank.
int __stdcall HapiBank_OpenAccount(HapiBank* bank, const char* name);
// Writes an integer item of the open account.
int __stdcall HapiBank_SetIntegerItem(HapiBank* bank, const char* name, int value);
// Reads an integer item of the open account, or def when it is missing.
int __stdcall HapiBank_GetIntegerItem(HapiBank* bank, const char* name, int def);

// Allocates size zeroed bytes in the state block, 8-byte aligned, and returns their address; a size the block cannot hold ends the game.
void* __cdecl SimAlloc(unsigned int size);
// Allocates like SimAlloc; the tag, a NUL-terminated name of the allocation, is for the analysis tools and changes nothing.
void* __cdecl SimAllocTagged(const char* tag, unsigned int size);
// Returns an allocation of SimAlloc or SimAllocTagged to the block; null, or an address that is not a live allocation, changes nothing.
void __cdecl SimFree(void* p);

// Appends a sim event of the kind to the sim event queue, its count arguments read from args, in records of six arguments each.
void __stdcall RecordSimEvent(SimEventKind kind, unsigned int flags, const unsigned int* args, unsigned int count);
// Reserves bytes zeroed bytes of the radar arena, writes their offset in the arena to offset and returns their address, or null when it is full.
void* SimCaptureAlloc(unsigned int bytes, unsigned int* offset);

// A table of the frames of a GAF animation: load data the host maps into the shared memory outside the block, which a module reads and never writes.
struct FrameTable;
// One frame of a GAF animation: its size, its offsets and its pixels.
struct GafFrame;
// The entry of a GAF animation whose frames a sequence steps through.
struct GafEntry;
// The cursor of a GAF sequence, in the block: the frame, the ticks left on it, whether it loops, and the entry.
struct Handle;
// Returns the frame of the given index of a frame table, or null when the index is outside the table.
GafFrame* __stdcall GetGafFrame(FrameTable* table, int index);
// Starts a sequence on an entry at a frame (the first when the index is outside the entry), with that frame's duration.
void __stdcall InitGafSequence(Handle* player, GafEntry* entry, int index);
// Advances a sequence by a tick and returns 1 when it moved to the next frame (or ended), 0 when it stayed or has no entry.
int __stdcall StepGafSequence(Handle* player);

// Returns the network id of a player slot, or -1 when the slot is 10 or holds no player.
int __stdcall GetSlotDpid(unsigned char index);
// Returns the player slot whose network id is id, or 10 when none has it or id is -1.
unsigned char __stdcall FindSlotByDpid(int id);

// Publishes an effect, its payload of size bytes, to the calling module's outbox, from which the engine's router copies it to each subscriber of its kind; a kind outside the caller's publishes table, or a size other than its payload's, is refused.
void __stdcall Emit(const void* effect, unsigned int size);

#endif
