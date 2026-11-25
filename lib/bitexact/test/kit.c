#include <stdio.h>
#include <string.h>

#include "kit.h"

#ifdef BX_KIT

#define KIT_ENTRIES 64

typedef struct {
    char name[32];
    FILE *file[2];
    int laid_out;
    long dropped;
} kit_entry;

static char kit_dir[512];
static int kit_active;
static kit_entry kit_entries[KIT_ENTRIES];
static int kit_count;
static FILE *kit_out;
static int kit_failed;

static void kit_fail(const char *what, const char *path)
{
    printf("kit: cannot %s %s\n", what, path);
    kit_failed = 1;
}

static void kit_path(char *path, size_t size, const char *entry, const char *suffix)
{
    size_t n = strlen(kit_dir);
    const char *sep = (n > 0 && (kit_dir[n - 1] == '/' || kit_dir[n - 1] == '\\')) ? "" : "/";
    if (strlen(kit_dir) + strlen(sep) + strlen(entry) + strlen(suffix) + 1 > size) {
        path[0] = 0;
        kit_fail("name a file for", entry);
        return;
    }
    strcpy(path, kit_dir);
    strcat(path, sep);
    strcat(path, entry);
    strcat(path, suffix);
}

static kit_entry *kit_find(const char *entry)
{
    int i;
    for (i = 0; i < kit_count; i++)
        if (strcmp(kit_entries[i].name, entry) == 0)
            return &kit_entries[i];
    if (kit_count >= KIT_ENTRIES || strlen(entry) >= sizeof(kit_entries[0].name)) {
        kit_fail("hold the entry", entry);
        return NULL;
    }
    strcpy(kit_entries[kit_count].name, entry);
    return &kit_entries[kit_count++];
}

void kit_open(const char *dir)
{
    kit_active = 0;
    kit_failed = 0;
    if (dir == NULL)
        return;
    if (strlen(dir) >= sizeof(kit_dir)) {
        kit_fail("use the directory", dir);
        return;
    }
    strcpy(kit_dir, dir);
    kit_active = 1;
}

void kit_layout(const char *entry, const char *lines)
{
    char path[640];
    kit_entry *e;
    FILE *f;
    if (!kit_active || (e = kit_find(entry)) == NULL || e->laid_out)
        return;
    e->laid_out = 1;
    kit_path(path, sizeof(path), entry, ".layout.txt");
    if (path[0] == 0)
        return;
    f = fopen(path, "wb");
    if (f == NULL) {
        kit_fail("open", path);
        return;
    }
    if (fputs(lines, f) < 0)
        kit_fail("write", path);
    if (fclose(f) != 0)
        kit_fail("close", path);
}

void kit_begin(const char *entry, int edge)
{
    char path[640];
    kit_entry *e;
    int k = edge ? 0 : 1;
    kit_out = NULL;
    if (!kit_active || (e = kit_find(entry)) == NULL)
        return;
    if (e->file[k] == NULL) {
        kit_path(path, sizeof(path), entry, edge ? ".edge.bin" : ".random.bin");
        if (path[0] == 0)
            return;
        e->file[k] = fopen(path, "wb");
        if (e->file[k] == NULL) {
            kit_fail("open", path);
            return;
        }
        setvbuf(e->file[k], NULL, _IOFBF, 1 << 16);
    }
    kit_out = e->file[k];
}

void kit_u32(bx_u32 value)
{
    unsigned char b[4];
    if (kit_out == NULL)
        return;
    b[0] = (unsigned char)value;
    b[1] = (unsigned char)(value >> 8);
    b[2] = (unsigned char)(value >> 16);
    b[3] = (unsigned char)(value >> 24);
    if (fwrite(b, 1, 4, kit_out) != 4)
        kit_fail("write a record of", kit_dir);
}

void kit_i32(int value)
{
    kit_u32((bx_u32)value);
}

void kit_f64(double value)
{
    union {
        double d;
        bx_u32 w[2];
    } b;
    b.d = value;
    kit_u32(b.w[0]);
    kit_u32(b.w[1]);
}

void kit_bytes(const void *data, size_t size)
{
    if (kit_out != NULL && size > 0 && fwrite(data, 1, size, kit_out) != size)
        kit_fail("write a record of", kit_dir);
}

void kit_str(const char *s)
{
    size_t n = strlen(s);
    kit_u32((bx_u32)n);
    kit_bytes(s, n);
}

void kit_dropped(const char *entry)
{
    kit_entry *e;
    if (kit_active && (e = kit_find(entry)) != NULL)
        e->dropped++;
}

void kit_end(void)
{
    kit_out = NULL;
}

int kit_close(void)
{
    char path[640];
    int i, k, failed;
    for (i = 0; i < kit_count; i++) {
        for (k = 0; k < 2; k++)
            if (kit_entries[i].file[k] != NULL) {
                if (ferror(kit_entries[i].file[k]) || fclose(kit_entries[i].file[k]) != 0)
                    kit_fail("finish a file of", kit_entries[i].name);
                kit_entries[i].file[k] = NULL;
            }
        if (kit_entries[i].dropped > 0) {
            FILE *f;
            kit_path(path, sizeof(path), kit_entries[i].name, ".dropped.txt");
            if (path[0] == 0)
                continue;
            f = fopen(path, "wb");
            if (f == NULL) {
                kit_fail("open", path);
                continue;
            }
            if (fprintf(f, "%ld\n", kit_entries[i].dropped) < 0)
                kit_fail("write", path);
            if (fclose(f) != 0)
                kit_fail("close", path);
        }
    }
    failed = kit_failed;
    kit_count = 0;
    kit_active = 0;
    kit_failed = 0;
    kit_out = NULL;
    return failed;
}

#endif
