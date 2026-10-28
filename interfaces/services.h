#ifndef INTERFACES_SERVICES_H
#define INTERFACES_SERVICES_H

#include <string.h>

// Major version of the services interface, changed when a declaration is removed or its signature changes.
#define SERVICES_VERSION_MAJOR 1
// Minor version of the services interface, changed when a declaration is added.
#define SERVICES_VERSION_MINOR 0

// Returns a random number in [0, range) from the main stream of the game logic.
int __stdcall RandomInt(int range);
// Shows the message in the window of the display and ends the program.
void __stdcall FatalError(char* message);
// Builds dir/name.ext below the data directory in buf and returns buf.
char* __stdcall BuildDataPath(char* buf, const char* dir, const char* name, const char* ext);
// The extension of TDF data files.
extern char g_extTdf[4];

#pragma pack(push, 8)
#include "../src/util/string_ref.h"

class TdfRecord;

// One key and value pair of a TDF record.
struct TdfField {
    StringRef key;
    StringRef value;

    // Builds a field from a key and a value.
    TdfField(const StringRef& a, const StringRef& b);
    // FUNCTION: 0x4c54a0 ??0TdfField@@QAE@ABU0@@Z
    TdfField(const TdfField& other)
        : key(other.key), value(other.value)
    {
    }
    // Releases the key and the value.
    ~TdfField();
    // Assigns a field from another one in a record copy.
    void* AssignPair(int* param_1);
};
#pragma pack(pop)

#pragma pack(push, 1)
// The child record list of a TDF record (a VC5 vector).
struct TdfRecordList {
    char unknown_0[4];
    TdfRecord** first;
    TdfRecord** last;
    TdfRecord** end;
};

// The field list of a TDF record (a VC5 vector).
struct TdfFieldList {
    char unknown_0[4];
    TdfField* first;
    TdfField* last;
    TdfField* end;
};

// The fields of a TDF record.
struct TdfEntries {
    char less;
    TdfFieldList v;
};

// One bracketed record of a TDF file: its fields and its child records.
class TdfRecord {
public:
    char* name;
    TdfRecordList children;
    TdfEntries entries;
    char* text;

    // Parses a record and its children from text, setting nextblock past it.
    TdfRecord(char* name, char* text, char** nextblock, char* filename);
    // Frees the record and its children.
    ~TdfRecord();

    // Returns the text between start and end without surrounding blanks.
    StringRef MakeTrimmedString(char* start, char* end);
    // Returns the child record with the given name, or null.
    TdfRecord* FindChild(char* key);
    // Returns the child record at an index.
    TdfRecord* GetChild(int index);
    // Adds the text between start and end to the file checksum.
    void ComputeRecordChecksum(char* start, char* end);
    // Returns the sub-record with the given name, or null.
    TdfRecord* FindSubRecord(const char* name);
    // Returns the record name.
    int GetRecordName();
    // Returns the name of the field at an index.
    char* GetFieldName(int index);
    // Returns the address of the value of a field, or 0 when it is missing.
    int FindFieldValue(char* name);
    // Reads a field as a fixed-point number into dst.
    int* GetFieldFixed(int* dst, char* key, int def);
    // Reads a field as an integer, or def when it is missing.
    int GetFieldInt(const char* name, int def);
    // Reads a field as a double, or def when it is missing.
    double GetFieldDouble(const char* name, double def);
    // Copies a field to dst, or def when it is missing.
    int GetFieldString(char* dst, char* key, size_t size, char* def);
    // Copies the record name to dest.
    void CopyRecordName(char* dest, size_t count);
    // Returns the number of sub-records.
    int GetSubRecordCount();
    // Returns the sub-record at an index.
    TdfRecord* GetSubRecord(int index);
    // Returns the number of fields.
    int GetFieldCount();
};
#pragma pack(pop)

// A parsed TDF data file with a current record selected.
class TdfFile {
public:
    TdfRecord* root;
    TdfRecord* current;
    int flags;

    // Builds an empty file.
    TdfFile();
    // Unloads the file.
    ~TdfFile();
    // Copies a field of the current record to dst, or def when it is missing.
    int GetString(char* dst, char* key, int size, char* def)
    {
        return current->GetFieldString(dst, key, size, def);
    }
    // Reads an integer field of the current record.
    int GetInt(char* key, int def) { return current->GetFieldInt(key, def); }
    // Reads a double field of the current record.
    double GetDouble(char* key, double def) { return current->GetFieldDouble(key, def); }
    // Returns the value of a field of the current record.
    char* GetValue(char* key) { return (char*)current->FindFieldValue(key); }
    // Loads and parses the file at a path.
    int LoadFile(char* path);
    // Removes the comments from TDF text in place.
    void StripComments(char* p);
    // Parses TDF text already in memory.
    void LoadBuffer(char* data, int size, int flag, char* path);
    // Frees the parsed records.
    void Unload();
    // Selects the record with the given name as the current one.
    int SelectRecord(char* name);
    // Selects the record at an index as the current one.
    int SelectRecordAt(int index);
    // Selects the root record again.
    void ResetCurrentRecord();
    // Returns the index of the current record.
    int GetCurrentRecord();
    // Sets the index of the current record.
    void SetCurrentRecord(int val);
};

#endif
