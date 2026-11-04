#include <windows.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../../interfaces/host.h"

struct HostDir {
    struct host_dir_entry* entries;
    unsigned int count;
    unsigned int next;
};

extern "C" void* HOST_CALL host_file_open_read(const char* path)
{
    return fopen(path, "rb");
}

extern "C" void* HOST_CALL host_file_create(const char* path, unsigned int flags)
{
    return fopen(path, (flags & HOST_FILE_TEXT) ? "w" : "w+b");
}

extern "C" void* HOST_CALL host_file_open_append(const char* path)
{
    return fopen(path, "a+b");
}

extern "C" unsigned int HOST_CALL host_file_read(void* file, void* buf, unsigned int size)
{
    return fread(buf, 1, size, (FILE*)file);
}

extern "C" unsigned int HOST_CALL host_file_write(void* file, const void* buf, unsigned int size)
{
    return fwrite(buf, 1, size, (FILE*)file);
}

extern "C" long HOST_CALL host_file_seek(void* file, long offset, int origin)
{
    if (fseek((FILE*)file, offset, origin) != 0)
        return -1;
    return ftell((FILE*)file);
}

extern "C" long HOST_CALL host_file_size(void* file)
{
    return _filelength(_fileno((FILE*)file));
}

extern "C" int HOST_CALL host_file_truncate(void* file, long size)
{
    return _chsize(_fileno((FILE*)file), size) == 0;
}

extern "C" void HOST_CALL host_file_close(void* file)
{
    fclose((FILE*)file);
}

static void FillEntry(const struct _finddata_t* fd, struct host_dir_entry* entry)
{
    memset(entry, 0, sizeof(*entry));
    strcpy(entry->name, fd->name);
    entry->attrib = fd->attrib;
    entry->size = fd->size;
    entry->time_write = (unsigned int)fd->time_write;
}

static int __cdecl CompareEntryNames(const void* a, const void* b)
{
    const unsigned char* x = (const unsigned char*)((const struct host_dir_entry*)a)->name;
    const unsigned char* y = (const unsigned char*)((const struct host_dir_entry*)b)->name;
    for (;; x++, y++) {
        int cx = (*x >= 'a' && *x <= 'z') ? *x - 32 : *x;
        int cy = (*y >= 'a' && *y <= 'z') ? *y - 32 : *y;
        if (cx != cy)
            return cx - cy;
        if (cx == 0)
            break;
    }
    return strcmp(((const struct host_dir_entry*)a)->name, ((const struct host_dir_entry*)b)->name);
}

extern "C" void* HOST_CALL host_dir_first(const char* pattern, struct host_dir_entry* entry)
{
    struct _finddata_t fd;
    long handle = _findfirst(pattern, &fd);
    if (handle == -1)
        return 0;
    HostDir* dir = (HostDir*)malloc(sizeof(HostDir));
    if (dir == 0) {
        _findclose(handle);
        return 0;
    }
    dir->entries = 0;
    dir->count = 0;
    dir->next = 1;
    unsigned int capacity = 0;
    do {
        if (dir->count == capacity) {
            capacity = capacity ? capacity * 2 : 64;
            struct host_dir_entry* grown = (struct host_dir_entry*)realloc(dir->entries, capacity * sizeof(struct host_dir_entry));
            if (grown == 0) {
                _findclose(handle);
                free(dir->entries);
                free(dir);
                return 0;
            }
            dir->entries = grown;
        }
        FillEntry(&fd, &dir->entries[dir->count++]);
    } while (_findnext(handle, &fd) != -1);
    _findclose(handle);
    qsort(dir->entries, dir->count, sizeof(struct host_dir_entry), CompareEntryNames);
    *entry = dir->entries[0];
    return dir;
}

extern "C" int HOST_CALL host_dir_next(void* dir, struct host_dir_entry* entry)
{
    HostDir* d = (HostDir*)dir;
    if (d->next >= d->count)
        return 0;
    *entry = d->entries[d->next++];
    return 1;
}

extern "C" void HOST_CALL host_dir_close(void* dir)
{
    free(((HostDir*)dir)->entries);
    free(dir);
}

extern "C" unsigned int HOST_CALL host_file_exe_path(char* buf, unsigned int size)
{
    return GetModuleFileNameA(NULL, buf, size);
}

extern "C" unsigned int HOST_CALL host_file_full_path(const char* name, char* buf, unsigned int size)
{
    char* filePart;
    return GetFullPathNameA(name, size, buf, &filePart);
}

extern "C" int HOST_CALL host_dir_set_current(const char* path)
{
    return SetCurrentDirectoryA(path) != 0;
}

extern "C" int HOST_CALL host_file_config_int(const char* path, const char* section, const char* key, int fallback)
{
    return (int)GetPrivateProfileIntA(section, key, fallback, path);
}

extern "C" unsigned int HOST_CALL host_file_config_string(const char* path, const char* section, const char* key, const char* fallback, char* buf, unsigned int size)
{
    return GetPrivateProfileStringA(section, key, fallback, buf, size, path);
}

extern "C" unsigned int HOST_CALL host_clipboard_text_get(void* owner, char* buf, unsigned int size)
{
    unsigned int copied = 0;
    if (OpenClipboard((HWND)owner)) {
        HANDLE mem = GetClipboardData(CF_TEXT);
        if (mem != NULL) {
            unsigned int length = (unsigned int)GlobalSize(mem);
            if (length != 0) {
                const char* src = (const char*)GlobalLock(mem);
                copied = length < size ? length : size;
                memcpy(buf, src, copied);
                GlobalUnlock(mem);
            }
        }
        CloseClipboard();
    }
    return copied;
}
