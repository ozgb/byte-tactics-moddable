#ifndef FILES_API_H
#define FILES_API_H

#include <vector>

#include "../../interfaces/seams.h"
#include "../util/engine_services.h"
#include "files_bank.h"
#include "file_handle.h"

static inline FileHandle* HAPI_OpenFileRead(const char* path)
{
    return g_seams.files->OpenRead(path);
}

static inline FileHandle* HAPI_OpenFileAppend(const char* path)
{
    return g_seams.files->OpenAppend(path);
}

static inline FileHandle* HAPI_CreateFile(const char* path)
{
    return g_seams.files->Create(path);
}

static inline int HAPI_CloseFile(FileHandle* file)
{
    return g_seams.files->Close(file);
}

static inline int HAPI_IsInArchive(FileHandle* file)
{
    return g_seams.files->IsInArchive(file);
}

static inline int HAPI_SeekFile(FileHandle* file, int pos)
{
    return g_seams.files->Seek(file, pos);
}

static inline int HAPI_TellFile(FileHandle* file)
{
    return g_seams.files->Tell(file);
}

static inline int HAPI_readfromfile(FileHandle* file, void* buf, int size)
{
    return g_seams.files->Read(file, buf, size);
}

static inline unsigned int HAPI_WriteFile(FileHandle* file, const void* buf, unsigned int size)
{
    return g_seams.files->Write(file, buf, size);
}

static inline int HAPI_FileLength(FileHandle* file)
{
    return g_seams.files->Length(file);
}

static inline int HAPI_FileLengthByName(const char* name)
{
    return g_seams.files->LengthByName(name);
}

static inline void* HAPI_ReadFileAt(const char* name, void* buffer, int pos, unsigned int size)
{
    return g_seams.files->ReadAt(name, buffer, pos, size);
}

static inline void* HAPI_LoadFile(const char* name, int* size)
{
    return g_seams.files->Load(name, size);
}

static inline void* HAPI_LoadOpenFile(const char* name, FileHandle* file, unsigned int* outSize)
{
    return g_seams.files->LoadOpenFile(name, file, outSize);
}

static inline int WriteBufferToFile(const char* filename, const void* data, int size)
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

static inline int ChangeDrive(const char* drive)
{
    return g_seams.files->ChangeDrive(drive);
}

static inline const char* GetDriveDirectory(const char* drive, char* buf, int size)
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

static inline void FindFilesRecursive(const char* path, const char* pat, std::vector<StringRef>* tree, int state, int recursive)
{
    g_seams.files->FindFilesRecursive(path, pat, (FileNameList*)tree, state, recursive);
}

static inline void MakeDirectoryPath(const char* path)
{
    g_seams.files->MakeDirectoryPath(path);
}

static inline void SaveStartDirectory(void)
{
    g_seams.files->SaveStartDirectory();
}

static inline void GetStartDirectory(char* out)
{
    g_seams.files->GetStartDirectory(out);
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

static inline int HAPI_IsInArchive(void* file)
{
    return HAPI_IsInArchive((FileHandle*)file);
}

static inline int HAPI_SeekFile(void* file, int pos)
{
    return HAPI_SeekFile((FileHandle*)file, pos);
}

static inline int HAPI_TellFile(void* file)
{
    return HAPI_TellFile((FileHandle*)file);
}

static inline int HAPI_readfromfile(void* file, void* buf, int size)
{
    return HAPI_readfromfile((FileHandle*)file, buf, size);
}

static inline unsigned int HAPI_WriteFile(void* file, const void* buf, unsigned int size)
{
    return HAPI_WriteFile((FileHandle*)file, buf, size);
}

static inline int HAPI_FileLength(void* file)
{
    return HAPI_FileLength((FileHandle*)file);
}

static inline void* HAPI_LoadOpenFile(const char* name, void* file, unsigned int* outSize)
{
    return HAPI_LoadOpenFile(name, (FileHandle*)file, outSize);
}

#endif
