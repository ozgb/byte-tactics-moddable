#ifndef INTERFACES_SERVICES_H
#define INTERFACES_SERVICES_H

#include <stdlib.h>
#include <string.h>

// Major version of the services interface, changed when a declaration is removed or its signature changes.
#define SERVICES_VERSION_MAJOR 1
// Minor version of the services interface, changed when a declaration is added.
#define SERVICES_VERSION_MINOR 2

// Returns a random number in [0, range) from the main stream of the game logic.
int __stdcall RandomInt(int range);
// Shows the message in the window of the display and ends the program.
void __stdcall FatalError(char* message);
// Builds dir/name.ext below the data directory in buf and returns buf.
char* __stdcall BuildDataPath(char* buf, const char* dir, const char* name, const char* ext);
// The extension of TDF data files.
extern char g_extTdf[4];

// Returns nonzero when the harness runs in test mode.
int HarnessTestMode();
// Puts the program in test mode.
void EnterHarnessTestMode();

struct Vec3;
struct Vec3f;
struct HeapEntry;

// Returns the heading of the horizontal vector from one position to another.
int __stdcall GetHeadingBetween(Vec3* from, Vec3* to);
// Returns the vector from one position to another.
Vec3f __stdcall VectorFromTo(Vec3f from, Vec3f to);
// Returns the cross product of two vectors.
Vec3f __stdcall CrossProduct(Vec3f a, Vec3f b);
// Returns a vector scaled to length one.
Vec3f __stdcall NormalizeVector(Vec3f v);
// Rotates a position by three angles and stores the result in out.
void __stdcall RotateByAngles(Vec3* in, Vec3* out, short* angles);
// Returns the checksum of a byte range.
int __stdcall ComputeChecksum(unsigned char* data, int len);

// Allocates a block of the game heap, naming the call site for the debug tables.
void* __cdecl GameAllocIgnoreTag(const char* name, unsigned int size);
// Copies a string into a new block of the game heap, or returns null for null.
char* __cdecl GameStrdup(char* s);
// Makes the pages of a block of the game heap read-only.
void __cdecl ProtectBlockReadOnly(void* p);
// Makes the pages of a block of the game heap writable.
void __cdecl ProtectBlockReadWrite(void* p);
// Copies one dword unless the source is null.
void __stdcall CopyDwordIfNonNull(int* dest, int* src);
// Arranges a range of path search entries into a heap.
void __stdcall MakeHeap(HeapEntry* first, HeapEntry* last, int*, HeapEntry*);

// Returns the translation of a key.
char* __stdcall Translate(const char* key);
// Returns the index of a translation key.
int __stdcall FindTranslation(char* name);
// Skips n lines of text and returns the rest.
char* __stdcall SkipTextLines(char* text, int n);
// Writes formatted text to a host file and returns its length, or -1 on failure.
int FilePrintf(void* file, const char* fmt, ...);
// Loads a whole data file into the game heap, counting the load progress.
char* __stdcall LoadFileWithProgress(char* path);
// Loads a 3DO model of the data directory and binds its textures.
void* __stdcall LoadObject3d(const char* name);
// Returns non-zero when the data files are read from archives.
int __stdcall UsesDataArchives(void);
// Returns the address of the preferred language name, or 0 when none is set.
int __stdcall GetPreferredLanguage(void);
// Writes the settings of the game to the host.
void __stdcall SaveSettings();
// Loads a GAF animation file of the data directory, once.
void* __stdcall LoadAnimGaf(char* name);

// The arguments of one console command.
class CommandArgs {
public:
    char* args[0x14];
    char buffer[0x7e];
    char unknown_ce[2];
    int count;

    // Returns the argument at an index, or the default when there is none.
    char* GetArg(int index, char* default_val);
    // Returns the argument at an index as an integer, or the default when there is none.
    int GetIntArg(int index, int default_val);
    // Returns the argument at an index as a float, or the default when there is none.
    float GetFloatArg(int index, float default_val);
    // Clears the arguments and returns the list.
    CommandArgs* InitArgs();
    // Splits the text up to end into arguments.
    void Tokenize(char* text, char* end);
    // Removes the first n arguments.
    void ShiftArgs(int n);
    // Replaces the numbered placeholders of the arguments with those of another list.
    void SubstituteArgs(CommandArgs* other);

    // Returns the argument at an index unparsed, or def when there is none.
    char* GetArgPtr(int index, char* def)
    {
        if (index < 0 || index >= count) return def;
        return args[index];
    }
};

// Runs the commands of a text and returns the number run.
int __stdcall ExecuteCommandText(char* text, int len, CommandArgs* vars, int mask);

// Writes values of up to 32 bits into a growing buffer of words, low bits first.
class BitWriter {
public:
    int index;
    int bit;
    int capacity;
    unsigned int* data;
    unsigned int buffer[0x100];

    // Starts an empty stream over the built-in buffer.
    BitWriter();
    // Frees the grown buffer, if the stream outgrew the built-in one.
    void FreeBuffer();
    // Doubles the capacity of the buffer.
    void GrowBuffer();
    // Appends the low bits of a value.
    void WriteBits(int value, int bits);
    // Overwrites one byte of the stream.
    void SetByteAt(int offset, unsigned char value);
};

// Reads values of up to 32 bits from a buffer of words, low bits first.
class BitReader {
public:
    unsigned int* data;
    int index;
    int bit;

    // Reads bits as an unsigned value.
    int ReadBits(int bits);
    // Reads bits as a signed value, extending the sign.
    int ReadSignedBits(int bits);

    // Reads one bit.
    int ReadBit()
    {
        int r = (data[index] & (1 << bit)) != 0;
        if (++bit == 32) {
            bit = 0;
            index++;
        }
        return r;
    }
};

#pragma pack(push, 8)
// A reference-counted string whose text follows a count word.
class StringRef {
public:
    union {
        char* p;
        char* data;
        char* ptr;
    };

    // Builds an empty string.
    StringRef();
    // Shares the text of another string.
    StringRef(const StringRef& other);
    // Copies a null-terminated text.
    StringRef(const char* text);
    // Copies the first len characters of a text.
    StringRef(const char* text, int len);
    // Releases the reference.
    ~StringRef() { ReleaseRef(); }
    // Shares the text of another string.
    StringRef& operator=(const StringRef& other)
    {
        Assign(&other);
        return *this;
    }
    // Drops the reference and frees the text when it was the last.
    void ReleaseRef();
    // Shares the text of another string and returns this.
    StringRef* Assign(const StringRef* other);
    // Appends the text of another string and returns this.
    StringRef* Append(const StringRef& other);
    // Converts the text to lower case and returns this.
    StringRef* MakeLower();
    // Converts the text to upper case and returns this.
    StringRef* MakeUpper();
    // Replaces the text with a copy of a null-terminated text and returns this.
    StringRef* AssignText(const char* text);
    // Returns the text, copying it first when it is shared.
    char* GetUnique()
    {
        int len = (int)strlen(data);
        if (*(int*)(data - 4) != 1) {
            int* block = (int*)malloc(len + 5);
            *block = 1;
            char* copy = (char*)(block + 1);
            strcpy(copy, data);
            (*(int*)(data - 4))--;
            if (*(int*)(data - 4) == 0) {
                free(data - 4);
            }
            data = copy;
            return copy;
        }
        return data;
    }
    // Returns nonzero when the text is empty.
    int IsEmpty() const;
    // Returns the characters from start up to end as a new string.
    StringRef SubString(int start, int end) const;
};

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
