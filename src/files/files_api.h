#ifndef FILES_API_H
#define FILES_API_H

#include <vector>

#include "../../interfaces/seams.h"
#include "../../interfaces/services.h"
#include "files_bank.h"
#include "file_handle.h"

static inline FileHandle* HAPI_OpenFileRead(char* path)
{
    return g_seams.files->OpenRead(path);
}

static inline FileHandle* HAPI_OpenFileAppend(char* path)
{
    return g_seams.files->OpenAppend(path);
}

static inline FileHandle* HAPI_CreateFile(char* path)
{
    return g_seams.files->Create(path);
}

static inline int HAPI_CloseFile(FileHandle* file)
{
    return g_seams.files->Close(file);
}

static inline bool HAPI_IsInArchive(FileHandle* file)
{
    return g_seams.files->IsInArchive(file);
}

static inline long HAPI_SeekFile(FileHandle* file, long pos)
{
    return g_seams.files->Seek(file, pos);
}

static inline long HAPI_TellFile(FileHandle* file)
{
    return g_seams.files->Tell(file);
}

static inline int HAPI_readfromfile(FileHandle* file, void* buf, int size)
{
    return g_seams.files->Read(file, buf, size);
}

static inline unsigned int HAPI_WriteFile(FileHandle* file, void* buf, unsigned int size)
{
    return g_seams.files->Write(file, buf, size);
}

static inline long HAPI_FileLength(FileHandle* file)
{
    return g_seams.files->Length(file);
}

static inline long HAPI_FileLengthByName(char* name)
{
    return g_seams.files->LengthByName(name);
}

static inline void* HAPI_ReadFileAt(char* name, void* buffer, long pos, unsigned int size)
{
    return g_seams.files->ReadAt(name, buffer, pos, size);
}

static inline void* HAPI_LoadFile(char* name, int* size)
{
    return g_seams.files->Load(name, size);
}

static inline void* HAPI_LoadOpenFile(char* name, FileHandle* file, unsigned int* outSize)
{
    return g_seams.files->LoadOpenFile(name, file, outSize);
}

static inline int WriteBufferToFile(char* filename, void* data, int size)
{
    return g_seams.files->WriteBuffer(filename, data, size);
}

static inline void RemoveFile(const char* path)
{
    g_seams.files->Remove(path);
}

static inline void RenameFile(const char* oldName, const char* newName)
{
    g_seams.files->Rename(oldName, newName);
}

static inline void GetCurrentDriveLetter(char* buf)
{
    g_seams.files->GetCurrentDriveLetter(buf);
}

static inline int ChangeDrive(char* drive)
{
    return g_seams.files->ChangeDrive(drive);
}

static inline char* GetDriveDirectory(char* drive, char* buf, int size)
{
    return g_seams.files->GetDriveDirectory(drive, buf, size);
}

static inline void ChangeDirectory(const char* path)
{
    g_seams.files->ChangeDirectory(path);
}

static inline int MatchWildcard(const char* str, const char* pat)
{
    return g_seams.files->MatchWildcard(str, pat);
}

static inline int HAPI_FindFirst(const char* path, void* fd, int state, char recursive)
{
    return g_seams.files->FindFirst(path, fd, state, recursive);
}

static inline int HAPI_FindNext(int handle, void* fd)
{
    return g_seams.files->FindNext(handle, fd);
}

static inline int HAPI_FindClose(int handle)
{
    return g_seams.files->FindClose(handle);
}

static inline int CountDirectoryEntries(const char* path, int flag)
{
    return g_seams.files->CountDirectoryEntries(path, flag);
}

static inline void ListDirectory(const char* path, int dirsOnly, std::vector<StringRef>* list)
{
    g_seams.files->ListDirectory(path, dirsOnly, (FileNameList*)list);
}

static inline void FindFilesRecursive(char* path, const char* pat, std::vector<StringRef>* tree, int state, int recursive)
{
    g_seams.files->FindFilesRecursive(path, pat, (FileNameList*)tree, state, recursive);
}

static inline void MakeDirectoryPath(char* path)
{
    g_seams.files->MakeDirectoryPath(path);
}

static inline void SaveStartDirectory(void)
{
    g_seams.files->SaveStartDirectory();
}

static inline void GetStartDirectory(char* param)
{
    g_seams.files->GetStartDirectory(param);
}

static inline void* HAPI_AddArchive(const char* name, int mode)
{
    return g_seams.files->AddArchive(name, mode);
}

static inline void HAPI_CloseArchive(OPENHAPIFILE* archive)
{
    g_seams.files->CloseArchive(archive);
}

static inline void HAPI_DropMissingArchives(void)
{
    g_seams.files->DropMissingArchives();
}

static inline void HAPI_ResolveShadowedFiles(void)
{
    g_seams.files->ResolveShadowedFiles();
}

static inline int LzssCompress(unsigned char* dest, unsigned char* src, int len)
{
    return g_seams.files->LzssCompress(dest, src, len);
}

static inline int LzssExpand(unsigned char* dest, unsigned char* src)
{
    return g_seams.files->LzssExpand(dest, src);
}

static inline int LzssDisablePreset(void)
{
    return g_seams.files->LzssDisablePreset();
}

static inline int LzssEnablePreset(void)
{
    return g_seams.files->LzssEnablePreset();
}

static inline int HAPI_CloseFile(void* file)
{
    return HAPI_CloseFile((FileHandle*)file);
}

static inline bool HAPI_IsInArchive(void* file)
{
    return HAPI_IsInArchive((FileHandle*)file);
}

static inline long HAPI_SeekFile(void* file, long pos)
{
    return HAPI_SeekFile((FileHandle*)file, pos);
}

static inline long HAPI_TellFile(void* file)
{
    return HAPI_TellFile((FileHandle*)file);
}

static inline int HAPI_readfromfile(void* file, void* buf, int size)
{
    return HAPI_readfromfile((FileHandle*)file, buf, size);
}

static inline unsigned int HAPI_WriteFile(void* file, void* buf, unsigned int size)
{
    return HAPI_WriteFile((FileHandle*)file, buf, size);
}

static inline long HAPI_FileLength(void* file)
{
    return HAPI_FileLength((FileHandle*)file);
}

static inline void* HAPI_LoadOpenFile(char* name, void* file, unsigned int* outSize)
{
    return HAPI_LoadOpenFile(name, (FileHandle*)file, outSize);
}

#endif
