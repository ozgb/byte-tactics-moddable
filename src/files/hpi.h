#ifndef HPI_H
#define HPI_H

struct ArchiveDirectory;
struct FileHandle;
struct HapiBuf;
struct OPENHAPIFILE;
struct _finddata_t;

FileHandle* __stdcall HAPI_OpenFile(char* filename, const char* mode);
int* __stdcall HAPI_LoadFileInto(char* name, int* buf);
void __stdcall GetDirectoryEntry(struct _finddata_t* fd, int index, const char* pattern, int flag);
int __stdcall SetLastDirectory(char* path);
void __stdcall GetLastDirectory(char* dst);
void __stdcall RestoreStartDirectory();
int __stdcall HAPI_CopyIntoFile(FileHandle* dst, char* name);
int __stdcall HAPI_PackageBuildNullStub(int, int);
unsigned int __stdcall HAPI_BuildArchiveDirectory(char* path, HapiBuf* out, int* total);
OPENHAPIFILE* __stdcall HAPI_OpenArchive(const char* name, int mode);
void __stdcall HAPI_RemoveArchive(OPENHAPIFILE* pRecord);
void __stdcall HAPI_ClearShadowFlags(ArchiveDirectory* list);
void __stdcall HAPI_MarkShadowedFiles(char* path, int state, int recursive);

#endif
