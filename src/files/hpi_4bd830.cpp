#include <stdio.h>
#include <string.h>
#include <io.h>
#include "../../interfaces/host.h"
#include "archive_entry.h"

#pragma pack(push, 1)
struct Node_004bd830 {
    char unknown_0[0xc];
    unsigned char obfuscate;
};

struct OPENHAPIFILE {
    void* fp;
    int pos;
    Node_004bd830* node;
    int count;
    int unknown_10;
    char name[0x100];
};

struct Info {
    long offset;
    unsigned int size;
    unsigned char compressed;
};
#pragma pack(pop)

#include "file_handle.h"

struct ArchiveDirectory;

#include "hpi.h"
#include "files_impl.h"
#include "sqsh.h"
#include "../debug/debug_lib.h"
#include "../graphics/object_pool.h"
void __stdcall HAPI_WriteArchiveData(char* path, char* base, int off, void* f,
                            void (__cdecl* cb)(unsigned), unsigned extra,
                            int key, int flags);

long __stdcall HAPI_FileLength(FileHandle* file)
{
    if (file->shared != 0)
        return file->info->size;
    if (file->fp != 0)
        return host_file_size(file->fp);
    return 0;
}

static inline int nblocks(unsigned w)
{
    return (0 != (int)w % 65536) + (int)w / 65536;
}

// FUNCTION: 0x4bd830
void __stdcall HAPI_WriteArchiveData(char* path, char* base, int off, void* f,
                            void (__cdecl* cb)(unsigned), unsigned extra,
                            int key, int flags)
{
    Info* info;
    int len;
    ArchiveEntry* e;
    unsigned size;
    char name[260];
    char full[260];
    unsigned char buffer[0x1000];
    int n, * table;
    int clen;
    unsigned remaining, i;
    FileHandle* file;
    unsigned char* pack;
    int j;
    unsigned char* data;
    unsigned packlen;

    strcpy(name, path);
    if (name[strlen(name) - 1] != '\\')
        strcat(name, "\\");

    long pos2;
    long pos;
    for (i = 0; i < *(unsigned*)(off + base); i++) {
        e = (ArchiveEntry*)(base + *(int*)(off + base + 4)) + i;
        strcpy(full, name);
        strcat(full, base + e->name);
        if ((e->flags & 1) != 0) {
            HAPI_WriteArchiveData(full, base, e->data, f, cb, extra, key, flags);
        } else {
            file = HAPI_OpenFile(full, "rb");
            info = (Info*)(base + e->data);
            info->offset = host_file_seek(f, 0, HOST_SEEK_CUR);
            info->size = HAPI_FileLength(file);
            info->compressed = (char)flags;
            if ((char)flags) {
                int blocks = nblocks(info->size);
                table = (int*)GameAllocIgnoreTag("Block Sizes", blocks * 4);
                host_file_write(f, table, blocks * 4);
                packlen = SquashMaxPackedSize(0x10000, flags & 0xff);
                pack = (unsigned char*)GameAllocIgnoreTag("Pack Buffer", packlen);
                data = (unsigned char*)GameAllocIgnoreTag("Data Buffer", 0x10000);
                remaining = info->size;
                for (n = 0; n < blocks; n++, remaining -= 0x10000) {
                    int chunk;
                    chunk = remaining >= 0x10000 ? 0x10000 : remaining;
                    HAPI_readfromfile(file, data, chunk);
                    clen = packlen;
                    SquashPack((Chunk_4d1820 *)pack, &clen, (char*)data, chunk, flags & 0xff, 1);
                    table[n] = clen;
                    pos = host_file_seek(f, 0, HOST_SEEK_CUR);
                    len = clen;
                    if ((char)key) {
                        for (j = 0; j < len; j++)
                            pack[j] = (unsigned char)~((char)j + (char)pos ^ (char)key ^ pack[j]);
                    }
                    host_file_write(f, pack, len);
                }
                host_file_seek(f, info->offset, HOST_SEEK_SET);
                pos2 = host_file_seek(f, 0, HOST_SEEK_CUR);
                if ((char)key) {
                    for (j = 0; j < blocks * 4; j++)
                        ((unsigned char*)table)[j] = (unsigned char)~((char)j
                            + ((char)pos2) ^ (char)key ^ ((unsigned char*)table)[j]);
                }
                host_file_write(f, table, blocks * 4);
                host_file_seek(f, 0, HOST_SEEK_END);
                GameFreeThunk(table);
                GameFreeThunk(pack);
                GameFreeThunk(data);
            } else {
                size = info->size;
                while (size > 0) {
                    int chunk = size >= 0x1000 ? 0x1000 : size;
                    HAPI_readfromfile(file, buffer, chunk);
                    pos = host_file_seek(f, 0, HOST_SEEK_CUR);
                    if ((char)key) {
                        for (j = 0; j < chunk; j++)
                            buffer[j] = (unsigned char)~(buffer[j] ^ ((char)pos + (char)j
                                            ^ ((char)key)));
                    }
                    host_file_write(f, buffer, chunk);
                    size -= chunk;
                }
            }
            if (file->shared != 0) {
                --file->shared->count;
                if (file->shared->count == 0 && file->shared->unknown_10 == 0) {
                    host_file_close(file->shared->fp);
                    file->shared->fp = 0;
                }
            } else {
                host_file_close(file->fp);
            }
            if (file->buffer != 0)
                GameFreeThunk(file->buffer);
            if (file->buffer2 != 0)
                GameFreeThunk(file->buffer2);
            GameFreeThunk(file);
            if (cb != 0)
                cb(5 + (unsigned)(90 * info->offset - *(int*)(8 + base) * 90) / extra);
        }
    }
}
