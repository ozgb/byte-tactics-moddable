#ifndef INTERFACES_SERVICES_H
#define INTERFACES_SERVICES_H

#include "rand_stream.h"

// Major version of the services interface, changed when a declaration is removed or its signature changes.
#define SERVICES_VERSION_MAJOR 2
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

struct WindMeteorsEventHeader;
// Publishes an effect of wind_meteors (interfaces/wind_meteors.h), its payload of size bytes, to its outbox, from which the engine's router copies it to each subscriber of its kind, and records it for the dump's wind_meteors events; called by wind_meteors from its runs.
void __stdcall WindMeteorsEmit(const WindMeteorsEventHeader* effect, unsigned int size);

#endif
