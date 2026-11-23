#ifndef FILES_IMPL_H
#define FILES_IMPL_H

#include "../../interfaces/files.h"
#include "hapi_bank.h"
#include "../util/engine_services.h"

#include <vector>

FileHandle* __stdcall HAPI_OpenFileRead(const char* path);
FileHandle* __stdcall HAPI_OpenFileAppend(const char* path);
FileHandle* __stdcall HAPI_CreateFile(const char* path);
int __stdcall HAPI_CloseFile(FileHandle* file);
int __stdcall HAPI_IsInArchive(FileHandle* file);
int __stdcall HAPI_SeekFile(FileHandle* file, int pos);
int __stdcall HAPI_TellFile(FileHandle* file);
int __stdcall HAPI_readfromfile(FileHandle* file, void* buf, int size);
unsigned int __stdcall HAPI_WriteFile(FileHandle* file, const void* buf, unsigned int size);
int __stdcall HAPI_FileLength(FileHandle* file);
int __stdcall HAPI_FileLengthByName(const char* name);
void* __stdcall HAPI_ReadFileAt(const char* name, void* buffer, int pos, unsigned int size);
void* __stdcall HAPI_LoadFile(const char* name, int* size);
void* __stdcall HAPI_LoadOpenFile(const char* name, FileHandle* file, unsigned int* outSize);
int __stdcall WriteBufferToFile(const char* filename, const void* data, int size);
void __stdcall RemoveFile(const char* path);
void __stdcall RenameFile(const char* oldName, const char* newName);
void __stdcall GetCurrentDriveLetter(char* buf);
int __stdcall ChangeDrive(const char* drive);
const char* __stdcall GetDriveDirectory(const char* drive, char* buf, int size);
void __stdcall ChangeDirectory(const char* path);
int __stdcall MatchWildcard(const char* str, const char* pat);
int __stdcall HAPI_FindFirst(const char* path, void* fd, int state, char recursive);
int __stdcall HAPI_FindNext(int handle, void* fd);
int __stdcall HAPI_FindClose(int handle);
int __stdcall CountDirectoryEntries(const char* path, int flag);
void __stdcall ListDirectory(const char* path, int dirsOnly, std::vector<StringRef>* list);
void __stdcall FindFilesRecursive(const char* path, const char* pat, std::vector<StringRef>* tree, int state, int recursive);
void __stdcall MakeDirectoryPath(const char* path);
void __stdcall SaveStartDirectory(void);
void __stdcall GetStartDirectory(char* out);
void* __stdcall HAPI_AddArchive(const char* name, int mode);
void __stdcall HAPI_CloseArchive(OPENHAPIFILE* archive);
void __stdcall HAPI_DropMissingArchives(void);
void __stdcall HAPI_ResolveShadowedFiles(void);
int __stdcall LzssCompress(unsigned char* dest, unsigned char* src, int len);
int __stdcall LzssExpand(unsigned char* dest, unsigned char* src);
int __stdcall LzssDisablePreset(void);
int __stdcall LzssEnablePreset(void);

#endif
