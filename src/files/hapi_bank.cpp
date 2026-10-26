#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <io.h>
#include <stdlib.h>

struct SafeDepositBox {
    int named;
    union {
        char* name;
        int number;
    };
    int size;
    int pos;
    char* data;
};

struct BankItem {
    char* name;
    int type;
    union {
        int value;
        double real;
        char* string;
    };
};

struct BankAccount {
    char* name;
    int itemCount;
    int boxCount;
    int currentBox;
    BankItem* items;
    SafeDepositBox* boxes;
};

struct AccountList {
    int count;
    BankAccount* accounts;
    int current;
};

#include "file_handle.h"
#include "../util/file_printf.h"
#include "../../interfaces/host.h"

#include "files_impl.h"

#include "../debug/debug_lib.h"
#include "../debug/debug_lib.h"
#include "../graphics/object_pool.h"

#include "sqsh.h"
#include "../graphics/display.h"
#include "../graphics/palette.h"

#pragma pack(push, 1)
struct BankFileHeader {
    char magic[8];
    int nameOffset;
    int poolOffset;
    int headerSize;
    int version;
    bool compressed;
    char unused_19[9];
};
#pragma pack(pop)

struct PoolImage {
    void* buf;
    int size;
};

struct StringPool {
    char* data;
    int len;
    int csize;
};

struct AccountHeader {
    int size;
    char* strOffset;
    int nInts;
    int nDoubles;
    int nStrings;
    int nBlobs;
    int compressed;
    int unknown_1c;
};

struct IntRecord {
    char* name;
    int value;
};

#pragma pack(push, 4)
struct DoubleRecord {
    char* name;
    double value;
};
#pragma pack(pop)

struct BoxRecord {
    char* name;
    int number;
    int offset;
    int len;
};

#include "hapi_bank.h"

// FUNCTION: 0x4b3620
AccountBank* AccountBank::InitBank()
{
    bank = 0;
    return this;
}

// FUNCTION: 0x4b3630
void AccountBank::CloseBank()
{
    if (bank != 0) {
        for (int i = 0; i < bank->count; i++) {
            BankAccount* account = &bank->accounts[i];
            GameFreeThunk(account->name);
            if (account->items != 0) {
                for (int j = 0; j < account->itemCount; j++) {
                    GameFreeThunk(account->items[j].name);
                    if (account->items[j].type == 3) {
                        GameFreeThunk(account->items[j].string);
                    }
                }
                GameFreeThunk(account->items);
            }
            if (account->boxes != 0) {
                for (int k = 0; k < account->boxCount; k++) {
                    if (account->boxes[k].named != 0) {
                        GameFreeThunk(account->boxes[k].name);
                    }
                    if (account->boxes[k].data != 0) {
                        GameFreeThunk(account->boxes[k].data);
                    }
                }
                GameFreeThunk(account->boxes);
            }
        }
        if (bank->accounts != 0) {
            GameFreeThunk(bank->accounts);
        }
        GameFreeThunk(bank);
    }
}

// FUNCTION: 0x4b3750
void AccountBank::NewBank()
{
    CloseBank();
    bank = (AccountList*)GameCalloc(1, 0xc);
    bank->current = -1;
}

// FUNCTION: 0x4b3770
int AccountBank::OpenBank(char* filename, char* name, char* account)
{
    void* raw;
    BankFileHeader h;
    PoolImage img;
    char errmsg[0x80];
    FileHandle* file;
    long remaining;
    long pos;

    file = HAPI_OpenFileRead(filename);
    if (file == 0) {
        return 0;
    }

    HAPI_readfromfile(file, &h, 0x22);
    if (strncmp(h.magic, "HAPIBANK", 8) != 0) {
        HAPI_CloseFile(file);
        return 0;
    }

    if (h.version != 1) {
        HAPI_CloseFile(file);
        return 0;
    }

    img.buf = 0;
    img.size = 0;
    remaining = HAPI_FileLength(file) - h.poolOffset;
    HAPI_SeekFile(file, h.poolOffset);

    if (h.compressed != 0) {
        void* src = GameAllocShared(remaining);
        int dsize;
        int err;

        HAPI_readfromfile(file, src, remaining);
        dsize = SquashUnpackedSize((unsigned char*)src);
        raw = GameAllocShared(dsize);
        err = SquashUnpack((char *)raw, (char *)src);
        if (err != 0) {
            sprintf(errmsg, "[AccountBank::OpenBank] Decompression Error: %s",
                    SquashErrorString(err));
            FatalError(errmsg);
        }
        img.buf = GameReallocIgnoreTag((int *)img.buf, dsize);
        img.size = dsize;
        memcpy(img.buf, raw, dsize);
        GameFreeThunk(raw);
        GameFreeThunk(src);
    } else {
        if (img.buf != 0) {
            GameFreeThunk(img.buf);
        }
        img.buf = GameAllocShared(remaining);
        img.size = remaining;
        HAPI_readfromfile(file, img.buf, remaining);
    }

    if (name != 0) {
        if (_strcmpi(name, (char*)img.buf + h.nameOffset) != 0) {
            HAPI_CloseFile(file);
            if (img.buf != 0) {
                GameFreeThunk(img.buf);
            }
            return 0;
        }
    }

    CloseBank();
    bank = (AccountList*)GameCalloc(1, 0xc);
    bank->current = -1;

    HAPI_SeekFile(file, h.headerSize);
    pos = HAPI_TellFile(file);
    if (pos < h.poolOffset) {
        do {
            LoadAccount(file, (int*)&img.buf, account);
            pos = HAPI_TellFile(file);
        } while (pos < h.poolOffset);
    }

    HAPI_CloseFile(file);
    if (img.buf != 0) {
        GameFreeThunk(img.buf);
    }
    return 1;
}

// FUNCTION: 0x4b39c0
int AccountBank::SaveBank(char* filename, char* name, int compress, int audit)
{
    char path[256];
    BankFileHeader header;
    int noff;
    int err;
    StringPool pool;
    void* file;
    int i;
    if (bank == 0 || bank->count == 0) { return 0; }
    if (audit) {
        strcpy(path, filename);
        StripExtension(path);
        strcat(path, ".cpa");
        WriteAuditFile(path);
    }
    file = host_file_create(filename, 0);
    if (file == 0) { return 0; }
    pool.data = 0;
    pool.len = 0;
    memset(&header, 0, sizeof(header));
    strncpy(header.magic, "HAPIBANK", 8);
    header.version = 1;
    int oldlen = pool.len;
    pool.len = oldlen + strlen(name) + 1;
    pool.data = (char*)GameReallocIgnoreTag((int *)pool.data, pool.len);
    strcpy(pool.data + oldlen, name);
    header.nameOffset = oldlen;
    header.headerSize = sizeof(header);
    host_file_write(file, &header, sizeof(header));
    for (i = 0; i < bank->count; i++) { SaveAccount(i, file, &pool, compress); }
    host_file_seek(file, 0, HOST_SEEK_END);
    header.poolOffset = host_file_seek(file, 0, HOST_SEEK_CUR);
    int dsize = pool.len;
    pool.csize = SquashMaxPackedSize(dsize, 2);
    int handle = SetOutOfMemoryHandler(0);
    char* cbuf = (char*)GameAllocShared(pool.csize);
    SetOutOfMemoryHandler(handle);
    if (cbuf != 0) { err = SquashPack((Chunk_4d1820 *)cbuf, &pool.csize, pool.data, dsize, 1, 0); }
    else { err = noff; }
    if (cbuf != 0 && err == 0 && pool.csize < dsize) {
        host_file_write(file, cbuf, pool.csize);
        header.compressed = true;
    } else {
        host_file_write(file, pool.data, pool.len);
    }
    host_file_seek(file, 0, HOST_SEEK_SET);
    host_file_write(file, &header, sizeof(header));
    host_file_close(file);
    if (cbuf != 0) { GameFreeThunk(cbuf); }
    if (pool.data != 0) { GameFreeThunk(pool.data); }
    return 1;
}

// FUNCTION: 0x4b3c60
void AccountBank::SaveAccount(int index, void* file, StringPool* buf, int compress)
{
    BankAccount* slot = &bank->accounts[index];
    if (slot->itemCount <= 0 && slot->boxCount <= 0) {
        return;
    }

    int off = host_file_seek(file, 0, HOST_SEEK_CUR);
    AccountHeader h;
    memset(&h, 0, sizeof(h));
    host_file_write(file, &h, sizeof(h));

    {
        char* name = slot->name;
        int oldlen = buf->len;
        buf->len += strlen(name) + 1;
        buf->data = (char*)GameReallocIgnoreTag((int *)buf->data, buf->len);
        strcpy(buf->data + oldlen, name);
        h.strOffset = (char*)oldlen;
    }

    int i;
    for (i = 0; i < slot->itemCount; i++) {
        if (slot->items[i].type == 1) {
            char* name = slot->items[i].name;
            int oldlen = buf->len;
            buf->len += strlen(name) + 1;
            buf->data = (char*)GameReallocIgnoreTag((int *)buf->data, buf->len);
            strcpy(buf->data + oldlen, name);
            int rec[2];
            rec[0] = oldlen;
            rec[1] = slot->items[i].value;
            host_file_write(file, rec, 8);
            h.nInts++;
        }
    }

    for (i = 0; i < slot->itemCount; i++) {
        if (slot->items[i].type == 2) {
            char* name = slot->items[i].name;
            int oldlen = buf->len;
            buf->len += strlen(name) + 1;
            buf->data = (char*)GameReallocIgnoreTag((int *)buf->data, buf->len);
            strcpy(buf->data + oldlen, name);
            int rec[3];
            rec[0] = oldlen;
            *(double*)&rec[1] = slot->items[i].real;
            host_file_write(file, rec, 0xc);
            h.nDoubles++;
        }
    }

    for (i = 0; i < slot->itemCount; i++) {
        if (slot->items[i].type == 3) {
            char* name = slot->items[i].name;
            int oldlen = buf->len;
            buf->len += strlen(name) + 1;
            buf->data = (char*)GameReallocIgnoreTag((int *)buf->data, buf->len);
            strcpy(buf->data + oldlen, name);
            int rec[2];
            rec[0] = oldlen;
            char* second = slot->items[i].string;
            int oldlen2 = buf->len;
            buf->len += strlen(second) + 1;
            buf->data = (char*)GameReallocIgnoreTag((int *)buf->data, buf->len);
            strcpy(buf->data + oldlen2, second);
            rec[1] = oldlen2;
            host_file_write(file, rec, 8);
            h.nStrings++;
        }
    }

    for (i = 0; i < slot->boxCount; i++) {
        if (slot->boxes[i].size > 0) {
            h.nBlobs++;
        }
    }

    int dataOffset = host_file_seek(file, 0, HOST_SEEK_CUR) + h.nBlobs * 0x10;
    for (i = 0; i < slot->boxCount; i++) {
        if (slot->boxes[i].size > 0) {
            int rec[4];
            if (slot->boxes[i].named != 0) {
                char* nm = slot->boxes[i].name;
                int oldlen = buf->len;
                buf->len += strlen(nm) + 1;
                buf->data = (char*)GameReallocIgnoreTag((int *)buf->data, buf->len);
                strcpy(buf->data + oldlen, nm);
                rec[0] = oldlen;
            } else {
                rec[0] = -1;
                rec[1] = slot->boxes[i].number;
            }
            rec[2] = dataOffset;
            int len = slot->boxes[i].size;
            rec[3] = len;
            dataOffset += len;
            host_file_write(file, rec, 0x10);
        }
    }

    for (i = 0; i < slot->boxCount; i++) {
        if (slot->boxes[i].size > 0) {
            host_file_write(file, slot->boxes[i].data, slot->boxes[i].size);
        }
    }

    h.size = host_file_seek(file, 0, HOST_SEEK_CUR) - off;

    if (compress != 0) {
        int handle = SetOutOfMemoryHandler(0);
        int len = h.size - 0x20;
        if (off < 0) len = h.size - 0x20;
        char* raw = (char*)GameAllocShared(len);
        if (raw != 0) {
            host_file_seek(file, off + 0x20, HOST_SEEK_SET);
            host_file_read(file, raw, len);
            int csize = SquashMaxPackedSize(len, 1);
            char* cbuf = (char*)GameAllocShared(csize);
            if (cbuf != 0) {
                int err = SquashPack((Chunk_4d1820 *)cbuf, &csize, raw, len, 1, 0);
                if (err == 0 && csize < len) {
                    host_file_seek(file, off + 0x20, HOST_SEEK_SET);
                    host_file_write(file, cbuf, csize);
                    h.size = csize + 0x20;
                    h.compressed = 1;
                    host_file_truncate(file, host_file_seek(file, 0, HOST_SEEK_CUR));
                }
                GameFreeThunk(cbuf);
            }
            GameFreeThunk(raw);
        }
        SetOutOfMemoryHandler(handle);
    }

    host_file_seek(file, off, HOST_SEEK_SET);
    host_file_write(file, &h, sizeof(h));
    host_file_seek(file, 0, HOST_SEEK_END);
}

// FUNCTION: 0x4b4270
void AccountBank::LoadAccount(FileHandle* fh, int* image, char* name)
{
    int buf;
    int len;
    int base;
    int end;
    int* p;
    AccountHeader h;
    char message[1000];

    base = (int)HAPI_TellFile(fh);
    HAPI_readfromfile(fh, &h, 0x20);
    end = base + h.size;
    if (name != 0 && _strcmpi(name, h.strOffset + *image) != 0) {
        HAPI_SeekFile(fh, end);
        return;
    }
    len = h.size - 0x20;
    if (len > 0) {
        if (h.compressed == 1) {
            char* tmp = (char*)GameAllocShared(len);
            HAPI_readfromfile(fh, tmp, len);
            buf = (int)GameAllocShared(SquashUnpackedSize((unsigned char*)tmp));
            int err = SquashUnpack((char *)buf, tmp);
            if (err != 0) {
                sprintf(message, "[AccountBank::LoadAccount] Decompression Error: %s\nFile: %s",
                        SquashErrorString(err), fh->name);
                FatalError(message);
            }
            GameFreeThunk(tmp);
        } else {
            buf = (int)GameAllocShared(len);
            HAPI_readfromfile(fh, (void*)buf, len);
        }
        OpenAccount(h.strOffset + *image);
        p = (int*)buf;
        {
            for (int i = 0; i < h.nInts; i++) {
                IntRecord rec = *(IntRecord*)p;
                p += 2;
                SetIntegerItem(rec.name + *image, rec.value);
            }
        }
        {
            for (int i = 0; i < h.nDoubles; i++) {
                DoubleRecord rec = *(DoubleRecord*)p;
                p += 3;
                SetDoubleItem(rec.name + *image, rec.value);
            }
        }
        {
            for (int i = 0; i < h.nStrings; i++) {
                int a = p[0];
                int b = p[1];
                p += 2;
                SetStringItem((char*)a + *image, (char*)b + *image);
            }
        }
        {
            for (int i = 0; i < h.nBlobs; i++) {
                BoxRecord rec = *(BoxRecord*)p;
                p += 4;
                if ((int)rec.name < 0)
                    OpenNumberedBox(rec.number);
                else
                    OpenNamedBox(rec.name + *image);
                char* src = (char*)(buf + (rec.offset - base) - 0x20);
                WriteBox(src, rec.len);
                SeekBox(0);
            }
        }
        HAPI_SeekFile(fh, end);
        GameFreeThunk((void*)buf);
    }
}

// FUNCTION: 0x4b4560
int AccountBank::OpenAccount(char* name)
{
    for (int i = 0; i < bank->count; i++) {
        if (_strcmpi(bank->accounts[i].name, name) == 0) {
            bank->current = i;
            bank->accounts[i].currentBox = -1;
            return 1;
        }
    }
    bank->current = bank->count;
    bank->count++;
    bank->accounts = (BankAccount*)GameReallocIgnoreTag(
        (int *)bank->accounts, bank->count * sizeof(BankAccount));
    memset(&bank->accounts[bank->current], 0, sizeof(BankAccount));
    bank->accounts[bank->current].name = GameStrdup(name);
    bank->accounts[bank->current].currentBox = -1;
    return 0;
}

// FUNCTION: 0x4b4630
int AccountBank::SetIntegerItem(const char* name, int value)
{
    if (bank && bank->current >= 0) {
        int i = FindItem(name, 1);
        if (bank->accounts[bank->current].items[i].type == 3)
            GameFreeThunk(bank->accounts[bank->current].items[i].string);
        bank->accounts[bank->current].items[i].value = value;
        bank->accounts[bank->current].items[i].type = 1;
        return 1;
    }
    return 0;
}

// FUNCTION: 0x4b46c0
int AccountBank::SetDoubleItem(const char* name, double value)
{
    if (bank && bank->current >= 0) {
        int i = FindItem(name, 1);
        if (bank->accounts[bank->current].items[i].type == 3)
            GameFreeThunk(bank->accounts[bank->current].items[i].string);
        bank->accounts[bank->current].items[i].real = value;
        bank->accounts[bank->current].items[i].type = 2;
        return 1;
    }
    return 0;
}

// FUNCTION: 0x4b4750
int AccountBank::SetStringItem(const char* name, char* value)
{
    if (bank && bank->current >= 0 && value) {
        int i = FindItem(name, 1);
        if (bank->accounts[bank->current].items[i].type == 3)
            GameFreeThunk(bank->accounts[bank->current].items[i].string);
        bank->accounts[bank->current].items[i].string = GameStrdup(value);
        bank->accounts[bank->current].items[i].type = 3;
        return 1;
    }
    return 0;
}

// FUNCTION: 0x4b4800
int AccountBank::GetIntegerItem(char* name, int def)
{
    if (bank && bank->current >= 0) {
        int i = FindItem(name, 0);
        if (i >= 0 && bank->accounts[bank->current].items[i].type == 1)
            return bank->accounts[bank->current].items[i].value;
    }
    return def;
}

// FUNCTION: 0x4b4850
double AccountBank::GetDoubleItem(char* name, double def)
{
    if (bank && bank->current >= 0) {
        int i = FindItem(name, 0);
        if (i >= 0 && bank->accounts[bank->current].items[i].type == 2)
            return bank->accounts[bank->current].items[i].real;
    }
    return def;
}

// FUNCTION: 0x4b48a0
char* AccountBank::GetStringItem(char* name, char* def)
{
    if (bank && bank->current >= 0) {
        int i = FindItem(name, 0);
        if (i >= 0 && bank->accounts[bank->current].items[i].type == 3)
            return bank->accounts[bank->current].items[i].string;
    }
    return def;
}

// FUNCTION: 0x4b48f0
int AccountBank::HasItem(const char* name)
{
    int i = FindItem(name, 0);
    return i >= 0 ? 1 : 0;
}

// FUNCTION: 0x4b4910
int AccountBank::FindItem(const char* name, int create)
{
    AccountList* f = bank;
    if (!f || f->current < 0)
        return -1;
    BankAccount* s = &f->accounts[f->current];
    for (int i = 0; i < s->itemCount; i++) {
        if (_strcmpi(s->items[i].name, name) == 0)
            return i;
    }
    if (!create)
        return -1;
    int n = s->itemCount;
    s->itemCount = n + 1;
    s->items = (BankItem*)GameReallocIgnoreTag((int *)s->items, (n + 1) * sizeof(BankItem));
    memset(&s->items[n], 0, sizeof(BankItem));
    s->items[n].name = GameStrdup((char*)name);
    return n;
}

// FUNCTION: 0x4b49d0
int AccountBank::FindNumberedBox(int number, int create)
{
    AccountList* t = bank;
    if (!t || t->current < 0)
        return -1;
    BankAccount* s = &t->accounts[t->current];
    for (int i = 0; i < s->boxCount; i++) {
        if (s->boxes[i].named == 0 && s->boxes[i].number == number)
            return i;
    }
    if (!create)
        return -1;
    int n = s->boxCount;
    s->boxCount = n + 1;
    s->boxes = (SafeDepositBox*)GameReallocIgnoreTag((int *)s->boxes, (n + 1) * sizeof(SafeDepositBox));
    memset(&s->boxes[n], 0, sizeof(SafeDepositBox));
    s->boxes[n].number = number;
    s->boxes[n].named = 0;
    return n;
}

// FUNCTION: 0x4b4a80
int AccountBank::FindNamedBox(char* name, int create)
{
    AccountList* t = bank;
    if (!t || t->current < 0)
        return -1;
    BankAccount* s = &t->accounts[t->current];
    for (int i = 0; i < s->boxCount; i++) {
        if (s->boxes[i].named && _strcmpi(s->boxes[i].name, name) == 0)
            return i;
    }
    if (!create)
        return -1;
    int n = s->boxCount;
    s->boxCount = n + 1;
    s->boxes = (SafeDepositBox*)GameReallocIgnoreTag((int *)s->boxes, (n + 1) * sizeof(SafeDepositBox));
    memset(&s->boxes[n], 0, sizeof(SafeDepositBox));
    s->boxes[n].name = GameStrdup(name);
    s->boxes[n].named = 1;
    return n;
}

// FUNCTION: 0x4b4b50
int AccountBank::OpenNumberedBox(int number)
{
    int r = FindNumberedBox(number, 1);
    bank->accounts[bank->current].currentBox = r;
    return bank->accounts[bank->current].boxes[r].data != 0;
}

// FUNCTION: 0x4b4ba0
int AccountBank::OpenNamedBox(char* name)
{
    int i = FindNamedBox(name, 1);
    bank->accounts[bank->current].currentBox = i;
    return bank->accounts[bank->current].boxes[i].data != 0;
}

// FUNCTION: 0x4b4bf0
int AccountBank::GetBoxSize()
{
    AccountList* p = bank;
    BankAccount* a = &p->accounts[p->current];
    int box = p->accounts[p->current].currentBox;
    return a->boxes[box].size;
}

// FUNCTION: 0x4b4c10
void AccountBank::SeekBox(int pos)
{
    BankAccount* s = &bank->accounts[bank->current];
    SafeDepositBox* c = &s->boxes[s->currentBox];
    if (pos < 0) {
        pos = 0;
    }
    if (pos > c->size) {
        pos = c->size;
    }
    c->pos = pos;
}

// FUNCTION: 0x4b4c50
int AccountBank::SeekBoxEnd()
{
    BankAccount* e = &bank->accounts[bank->current];
    SafeDepositBox* it = &e->boxes[e->currentBox];
    it->pos = it->size;
    return it->pos;
}

// FUNCTION: 0x4b4c80
int AccountBank::ReadBox(void* dst, int len)
{
    SafeDepositBox* c = &bank->accounts[bank->current].boxes[bank->accounts[bank->current].currentBox];
    int avail = c->size - c->pos;
    if (avail <= 0) {
        return 0;
    }
    if (len > avail) {
        len = avail;
    }
    memcpy(dst, c->data + c->pos, len);
    c->pos += len;
    return len;
}

// FUNCTION: 0x4b4cf0
int AccountBank::WriteBox(void* src, int len)
{
    SafeDepositBox* c = &bank->accounts[bank->current].boxes[bank->accounts[bank->current].currentBox];
    int need = len + c->pos;
    if (need > c->size) {
        c->data = (char*)GameReallocIgnoreTag((int *)c->data, need);
        c->size = need;
    }
    memcpy(c->data + c->pos, src, len);
    c->pos += len;
    return len;
}

// FUNCTION: 0x4b4d70
void AccountBank::WriteAuditFile(char* filename)
{
    void* file = host_file_create(filename, HOST_FILE_TEXT);
    int i;

    if (file == 0) {
        return;
    }

    FilePrintf(file, "HapiBank Audit File\n\n");
    FilePrintf(file, "Number of accounts: %i\n\n", bank->count);

    for (i = 0; i < bank->count; i++) {
        BankAccount* account = (BankAccount*)((char*)bank->accounts + i * 0x18);
        int j;

        FilePrintf(file, "Account:  \"%s\"\n{\n", account->name);
        for (j = 0; j < account->itemCount; j++) {
            BankItem* item = (BankItem*)((char*)account->items + j * 0x10);

            switch (item->type) {
            case 1:
                FilePrintf(file, "   Item \"%s\" Integer:  %i (0x%08x)\n", item->name, item->value, item->value);
                break;
            case 2:
                FilePrintf(file, "   Item \"%s\" Double:  %f\n", item->name, item->real);
                break;
            case 3:
                FilePrintf(file, "   Item \"%s\" String:  \"%s\"\n", item->name, item->string);
                break;
            default:
                FilePrintf(file, "   Item \"%s\" Uninitialized or Unknown\n", item->name);
                break;
            }
        }

        FilePrintf(file, "\n");
        for (j = 0; j < account->boxCount; j++) {
            SafeDepositBox* box = (SafeDepositBox*)((char*)account->boxes + j * 0x14);

            if (box->named != 0) {
                FilePrintf(file, "   Safe-Deposit Box \"%s\" contains %i bytes\n", box->name, box->size);
            } else {
                FilePrintf(file, "   Safe-Deposit Box #%i contains %i bytes\n", box->number, box->size);
            }
        }
        FilePrintf(file, "}\n\n");
    }

    FilePrintf(file, "----------EOF----------\n");
    host_file_close(file);
}
