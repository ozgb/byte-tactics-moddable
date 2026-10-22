#ifndef INTERFACES_FILES_H
#define INTERFACES_FILES_H

struct FileHandle;
struct OPENHAPIFILE;
class HapiBank;
struct FileNameList;

// Major version of the files interface, changed when a slot is removed or its signature changes.
#define FILES_VERSION_MAJOR 1
// Minor version of the files interface, changed when a slot is added.
#define FILES_VERSION_MINOR 0

// Game data files, loose or inside archives, the account files of saved games, and the SQSH packer.
struct FilesSeam {
    // Opens a file for reading, loose or from an archive, and returns its handle or 0. Hooks: none.
    FileHandle* (__stdcall* OpenRead)(char* path);
    // Opens a file in append mode and returns its handle or 0. Hooks: none.
    FileHandle* (__stdcall* OpenAppend)(char* path);
    // Creates a file for writing and returns its handle or 0. Hooks: none.
    FileHandle* (__stdcall* Create)(char* path);
    // Closes a file handle and returns 0 on success. Hooks: none.
    int (__stdcall* Close)(FileHandle* file);
    // Returns 1 when the file is read from an archive and 0 when it is loose on disk. Hooks: none.
    bool (__stdcall* IsInArchive)(FileHandle* file);
    // Moves the read position of a file and returns the new position. Hooks: none.
    long (__stdcall* Seek)(FileHandle* file, long pos);
    // Returns the read position of a file. Hooks: none.
    long (__stdcall* Tell)(FileHandle* file);
    // Reads up to size bytes from a file into buf and returns the number read. Hooks: none.
    int (__stdcall* Read)(FileHandle* file, void* buf, int size);
    // Writes size bytes to a loose file and returns the number written, or 0xffffffff for an archived file. Hooks: none.
    unsigned int (__stdcall* Write)(FileHandle* file, void* buf, unsigned int size);
    // Returns the size of an open file in bytes. Hooks: none.
    long (__stdcall* Length)(FileHandle* file);
    // Returns the size in bytes of the named file, or -1 when it is missing. Hooks: none.
    long (__stdcall* LengthByName)(char* name);
    // Reads size bytes from the given position of the named file into buffer and returns buffer, or 0. Hooks: none.
    void* (__stdcall* ReadAt)(char* name, void* buffer, long pos, unsigned int size);
    // Loads the whole named file into a new buffer, stores its size in size when given, and returns the buffer or 0. Hooks: none.
    void* (__stdcall* Load)(char* name, int* size);
    // Loads the whole named file through an already open handle, stores its size in outSize and returns the buffer or 0. Hooks: none.
    void* (__stdcall* LoadOpenFile)(char* name, FileHandle* file, unsigned int* outSize);
    // Writes a buffer to a new loose file and returns 1 on success. Hooks: none.
    int (__stdcall* WriteBuffer)(char* filename, void* data, int size);
    // Deletes a loose file. Hooks: none.
    void (__stdcall* Remove)(const char* path);
    // Renames a loose file. Hooks: none.
    void (__stdcall* Rename)(const char* oldName, const char* newName);
    // Writes the letter of the current drive into buf. Hooks: none.
    void (__stdcall* GetCurrentDriveLetter)(char* buf);
    // Makes the given drive current and returns 0 on success. Hooks: none.
    int (__stdcall* ChangeDrive)(char* drive);
    // Writes the current directory of the given drive into buf and returns buf. Hooks: none.
    char* (__stdcall* GetDriveDirectory)(char* drive, char* buf, int size);
    // Makes the given directory current. Hooks: none.
    void (__stdcall* ChangeDirectory)(const char* path);
    // Returns 1 when a name matches a pattern with * and ? wildcards, ignoring case. Hooks: none.
    int (__stdcall* MatchWildcard)(const char* str, const char* pat);
    // Starts a directory search over the disk (state -1) or the archives, fills fd with the first entry and returns a search handle or -1. Hooks: none.
    int (__stdcall* FindFirst)(const char* path, void* fd, int state, char recursive);
    // Fills fd with the next entry of a search and returns 0, or -1 when there are no more. Hooks: none.
    int (__stdcall* FindNext)(int handle, void* fd);
    // Ends a search and frees its handle. Hooks: none.
    int (__stdcall* FindClose)(int handle);
    // Returns the number of entries under a path, or only the directories when flag is set. Hooks: none.
    int (__stdcall* CountDirectoryEntries)(const char* path, int flag);
    // Appends the names under a path to list, or only the directories when dirsOnly is set. Hooks: none.
    void (__stdcall* ListDirectory)(const char* path, int dirsOnly, FileNameList* list);
    // Appends the full paths of the files under a path that match a pattern, descending into subdirectories. Hooks: none.
    void (__stdcall* FindFilesRecursive)(char* path, const char* pat, FileNameList* tree, int state, int recursive);
    // Creates every directory along a path. Hooks: none.
    void (__stdcall* MakeDirectoryPath)(char* path);
    // Stores the current directory as the start directory. Hooks: none.
    void (__stdcall* SaveStartDirectory)(void);
    // Copies the stored start directory into param. Hooks: none.
    void (__stdcall* GetStartDirectory)(char* param);
    // Opens an archive and adds it to the search order, returning it or 0 when it is already added or missing. Hooks: none.
    void* (__stdcall* AddArchive)(const char* name, int mode);
    // Closes an archive and removes it from the search order. Hooks: none.
    void (__stdcall* CloseArchive)(OPENHAPIFILE* archive);
    // Drops the archives whose files are missing from the search order. Hooks: none.
    void (__stdcall* DropMissingArchives)(void);
    // Marks the archive entries that a later archive or the disk overrides. Hooks: none.
    void (__stdcall* ResolveShadowedFiles)(void);
    // Compresses len bytes of src into dest and returns the compressed size. Hooks: none.
    int (__stdcall* LzssCompress)(unsigned char* dest, unsigned char* src, int len);
    // Expands a compressed buffer src into dest and returns the expanded size. Hooks: none.
    int (__stdcall* LzssExpand)(unsigned char* dest, unsigned char* src);
    // Stops the compressor and expander using the preset window. Hooks: none.
    int (__stdcall* LzssDisablePreset)(void);
    // Makes the compressor and expander use the preset window. Hooks: none.
    int (__stdcall* LzssEnablePreset)(void);
    // Clears a bank for use and returns it (thiscall in 3.1c, this first). Hooks: none.
    HapiBank* (__stdcall* BankInit)(HapiBank* self);
    // Frees a bank's accounts (thiscall in 3.1c, this first). Hooks: none.
    void (__stdcall* BankClose)(HapiBank* self);
    // Empties a bank (thiscall in 3.1c, this first). Hooks: none.
    void (__stdcall* BankNew)(HapiBank* self);
    // Loads a bank from a saved game file and opens an account, returning 0 on success (thiscall in 3.1c, this first). Hooks: none.
    int (__stdcall* BankOpen)(HapiBank* self, char* filename, char* name, char* account);
    // Saves a bank to a saved game file and returns 0 on success (thiscall in 3.1c, this first). Hooks: none.
    int (__stdcall* BankSave)(HapiBank* self, char* filename, char* name, int compress, int audit);
    // Makes the named account current and returns 0 on success (thiscall in 3.1c, this first). Hooks: none.
    int (__stdcall* BankOpenAccount)(HapiBank* self, char* name);
    // Stores an integer item in the current account (thiscall in 3.1c, this first). Hooks: none.
    int (__stdcall* BankSetInteger)(HapiBank* self, const char* name, int value);
    // Stores a double item in the current account (thiscall in 3.1c, this first). Hooks: none.
    int (__stdcall* BankSetDouble)(HapiBank* self, const char* name, double value);
    // Stores a string item in the current account (thiscall in 3.1c, this first). Hooks: none.
    int (__stdcall* BankSetString)(HapiBank* self, const char* name, char* value);
    // Returns an integer item of the current account, or def when it is missing (thiscall in 3.1c, this first). Hooks: none.
    int (__stdcall* BankGetInteger)(HapiBank* self, char* name, int def);
    // Returns a double item of the current account, or def when it is missing (thiscall in 3.1c, this first). Hooks: none.
    double (__stdcall* BankGetDouble)(HapiBank* self, char* name, double def);
    // Returns a string item of the current account, or def when it is missing (thiscall in 3.1c, this first). Hooks: none.
    char* (__stdcall* BankGetString)(HapiBank* self, char* name, char* def);
    // Returns 1 when the current account holds the named item (thiscall in 3.1c, this first). Hooks: none.
    int (__stdcall* BankHasItem)(HapiBank* self, const char* name);
    // Opens the numbered box of the current account for reading (thiscall in 3.1c, this first). Hooks: none.
    int (__stdcall* BankOpenNumberedBox)(HapiBank* self, int number);
    // Opens the named box of the current account for reading (thiscall in 3.1c, this first). Hooks: none.
    int (__stdcall* BankOpenNamedBox)(HapiBank* self, char* name);
    // Returns the size of the open box (thiscall in 3.1c, this first). Hooks: none.
    int (__stdcall* BankGetBoxSize)(HapiBank* self);
    // Moves the position in the open box (thiscall in 3.1c, this first). Hooks: none.
    void (__stdcall* BankSeekBox)(HapiBank* self, int pos);
    // Reads len bytes from the open box into dst and returns the number read (thiscall in 3.1c, this first). Hooks: none.
    int (__stdcall* BankReadBox)(HapiBank* self, void* dst, int len);
    // Writes len bytes from src to the open box and returns the number written (thiscall in 3.1c, this first). Hooks: none.
    int (__stdcall* BankWriteBox)(HapiBank* self, void* src, int len);
};

#endif
