#include <stdio.h>
#include <string.h>

#include "kit.h"

#ifdef BX_KIT

#define KIT_ENTRIES 64

typedef struct {
    char name[32];
    FILE *file[2];
    int laid_out;
} kit_entry;

static char kit_dir[512];
static int kit_active;
static kit_entry kit_entries[KIT_ENTRIES];
static int kit_count;
static FILE *kit_out;

static void kit_path(char *path, size_t size, const char *entry, const char *suffix)
{
    size_t n = strlen(kit_dir);
    const char *sep = (n > 0 && (kit_dir[n - 1] == '/' || kit_dir[n - 1] == '\\')) ? "" : "/";
    if (strlen(kit_dir) + strlen(sep) + strlen(entry) + strlen(suffix) + 1 > size) {
        path[0] = 0;
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
    if (kit_count >= KIT_ENTRIES || strlen(entry) >= sizeof(kit_entries[0].name))
        return NULL;
    strcpy(kit_entries[kit_count].name, entry);
    return &kit_entries[kit_count++];
}

void kit_open(const char *dir)
{
    kit_active = 0;
    if (dir == NULL || strlen(dir) >= sizeof(kit_dir))
        return;
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
    f = fopen(path, "wb");
    if (f == NULL) {
        printf("cannot open kit file %s\n", path);
        return;
    }
    fputs(lines, f);
    fclose(f);
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
        e->file[k] = fopen(path, "wb");
        if (e->file[k] == NULL) {
            printf("cannot open kit file %s\n", path);
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
    fwrite(b, 1, 4, kit_out);
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
    if (kit_out != NULL && size > 0)
        fwrite(data, 1, size, kit_out);
}

void kit_str(const char *s)
{
    size_t n = strlen(s);
    kit_u32((bx_u32)n);
    kit_bytes(s, n);
}

void kit_end(void)
{
    kit_out = NULL;
}

void kit_close(void)
{
    int i, k;
    for (i = 0; i < kit_count; i++)
        for (k = 0; k < 2; k++)
            if (kit_entries[i].file[k] != NULL)
                fclose(kit_entries[i].file[k]);
    kit_count = 0;
    kit_active = 0;
    kit_out = NULL;
}

#endif
