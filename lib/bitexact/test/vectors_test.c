#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../bitexact.h"
#include "../bx_game.h"
#include "../bx_libcmt.h"
#include "../bx_stl.h"

typedef struct {
    const unsigned char *p;
    const unsigned char *end;
    int short_read;
} reader_t;

typedef struct {
    const char *name;
    int (*run)(reader_t *r);
    const char *in;
    const char *out;
} entry_t;

static bx_u32 rd_u32(reader_t *r)
{
    bx_u32 v;
    if (r->end - r->p < 4) {
        r->short_read = 1;
        r->p = r->end;
        return 0;
    }
    v = (bx_u32)r->p[0] | ((bx_u32)r->p[1] << 8) | ((bx_u32)r->p[2] << 16) | ((bx_u32)r->p[3] << 24);
    r->p += 4;
    return v;
}

static int rd_i32(reader_t *r)
{
    return (int)rd_u32(r);
}

static double rd_f64(reader_t *r)
{
    union {
        double d;
        bx_u32 w[2];
    } b;
    b.w[0] = rd_u32(r);
    b.w[1] = rd_u32(r);
    return b.d;
}

static const unsigned char *rd_bytes(reader_t *r, size_t n)
{
    const unsigned char *p = r->p;
    if ((size_t)(r->end - r->p) < n) {
        r->short_read = 1;
        r->p = r->end;
        return NULL;
    }
    r->p += n;
    return p;
}

static char *rd_text(reader_t *r)
{
    size_t n = rd_u32(r);
    const unsigned char *p = rd_bytes(r, n);
    char *s;
    if (p == NULL)
        return NULL;
    s = (char *)malloc(n + 1);
    if (s == NULL)
        return NULL;
    memcpy(s, p, n);
    s[n] = 0;
    return s;
}

static bx_u64 bits_of(double d)
{
    union {
        double d;
        bx_u32 w[2];
    } b;
    b.d = d;
    return ((bx_u64)b.w[1] << 32) | b.w[0];
}

static bx_u32 bits_of_float(float f)
{
    union {
        float f;
        bx_u32 u;
    } b;
    b.f = f;
    return b.u;
}

static float float_from_bits(bx_u32 v)
{
    union {
        float f;
        bx_u32 u;
    } b;
    b.u = v;
    return b.f;
}

static int run_sine(reader_t *r)
{
    bx_u32 angle = rd_u32(r);
    int want = rd_i32(r);
    return bx_FixedSine((bx_u16)angle) != want;
}

static int run_cosine(reader_t *r)
{
    bx_u32 angle = rd_u32(r);
    int want = rd_i32(r);
    return bx_fixed_cos((bx_u16)angle) != want;
}

static int run_mul_hi(reader_t *r)
{
    int a = rd_i32(r);
    int b = rd_i32(r);
    int want = rd_i32(r);
    return bx_mul_hi32(a, b) != want;
}

static int run_scaled_sine(reader_t *r)
{
    bx_u32 angle = rd_u32(r);
    int scale = rd_i32(r);
    int want = rd_i32(r);
    return bx_ScaledSin((bx_i16)angle, scale) != want;
}

static int run_scaled_cosine(reader_t *r)
{
    bx_u32 angle = rd_u32(r);
    int scale = rd_i32(r);
    int want = rd_i32(r);
    return bx_ScaledCos((bx_i16)angle, scale) != want;
}

static int run_angle_of_vector(reader_t *r)
{
    int x = rd_i32(r);
    int z = rd_i32(r);
    int want = rd_i32(r);
    return bx_AngleOfVector(x, z) != want;
}

static int run_rotate_point(reader_t *r)
{
    bx_u32 angle = rd_u32(r);
    int xz[2];
    int want[2];
    xz[0] = rd_i32(r);
    xz[1] = rd_i32(r);
    want[0] = rd_i32(r);
    want[1] = rd_i32(r);
    bx_RotatePoint2D((bx_i16)angle, xz);
    return xz[0] != want[0] || xz[1] != want[1];
}

static int run_rotation_matrix(reader_t *r)
{
    bx_i16 angles[3];
    bx_i32 m[9];
    int want[9];
    int i;
    for (i = 0; i < 3; i++)
        angles[i] = (bx_i16)rd_u32(r);
    for (i = 0; i < 9; i++)
        want[i] = rd_i32(r);
    bx_build_rotation_matrix(angles, m);
    for (i = 0; i < 9; i++)
        if (m[i] != want[i])
            return 1;
    return 0;
}

static int run_apply_rotation(reader_t *r)
{
    bx_i32 m[9];
    bx_i32 in[3];
    bx_i32 out[3];
    int want[3];
    int aliased, i;
    for (i = 0; i < 9; i++)
        m[i] = rd_i32(r);
    for (i = 0; i < 3; i++)
        in[i] = rd_i32(r);
    aliased = rd_i32(r);
    for (i = 0; i < 3; i++)
        want[i] = rd_i32(r);
    if (aliased) {
        bx_apply_rotation_matrix(m, in, in);
        memcpy(out, in, sizeof(out));
    } else {
        bx_apply_rotation_matrix(m, in, out);
    }
    for (i = 0; i < 3; i++)
        if (out[i] != want[i])
            return 1;
    return 0;
}

static int run_mul_div(reader_t *r)
{
    int a = rd_i32(r);
    int b = rd_i32(r);
    int c = rd_i32(r);
    int want = rd_i32(r);
    return bx_MulDivTrunc(a, b, c) != want;
}

static int run_sqrt(reader_t *r)
{
    double x = rd_f64(r);
    double want = rd_f64(r);
    return bits_of(bx_sqrt(x)) != bits_of(want);
}

static int run_sqrt_float(reader_t *r)
{
    float f = float_from_bits(rd_u32(r));
    bx_u32 want = rd_u32(r);
    return bits_of_float((float)bx_sqrt((double)f)) != want;
}

static int run_ftol(reader_t *r)
{
    double x = rd_f64(r);
    int want = rd_i32(r);
    return bx_ftol(x) != want;
}

static int run_hypot(reader_t *r)
{
    double x = rd_f64(r);
    double y = rd_f64(r);
    double want = rd_f64(r);
    return bits_of(bx_hypot(x, y)) != bits_of(want);
}

static int run_acos(reader_t *r)
{
    double x = rd_f64(r);
    double want = rd_f64(r);
    return bits_of(bx_acos(x)) != bits_of(want);
}

static int run_float_text(reader_t *r)
{
    char *text = rd_text(r);
    bx_u32 want = rd_u32(r);
    int bad;
    if (text == NULL)
        return 1;
    bad = bits_of_float(bx_strtof_msvc5(text, 0)) != want;
    free(text);
    return bad;
}

static int run_double_text(reader_t *r)
{
    char *text = rd_text(r);
    double want = rd_f64(r);
    int bad;
    if (text == NULL)
        return 1;
    bad = bits_of(bx_atof_msvc5(text)) != bits_of(want);
    free(text);
    return bad;
}

static int same_next(reader_t *r, bx_rand_state *s)
{
    int k, bad = 0;
    for (k = 0; k < 4; k++)
        if (bx_rand(s) != rd_i32(r))
            bad = 1;
    return bad;
}

static int run_rand(reader_t *r)
{
    bx_rand_state s;
    bx_u32 seed = rd_u32(r);
    int i, bad = 0;
    bx_srand(&s, seed);
    for (i = 0; i < 10000; i++)
        if (bx_rand(&s) != rd_i32(r))
            bad = 1;
    return bad;
}

static int run_shuffle(reader_t *r)
{
    bx_rand_state s;
    bx_u32 seed = rd_u32(r);
    int size = rd_i32(r);
    size_t bytes = rd_u32(r);
    const unsigned char *order = rd_bytes(r, bytes);
    int *a;
    int i, bad = 0;
    if (order == NULL || size < 0 || bytes != (size_t)size * 4)
        return 1;
    a = (int *)malloc((size_t)size * sizeof(int) + sizeof(int));
    if (a == NULL)
        return 1;
    for (i = 0; i < size; i++)
        a[i] = i * 3 + 1;
    bx_srand(&s, seed);
    bx_random_shuffle_int(&s, a, a + size);
    for (i = 0; i < size; i++) {
        bx_u32 v = (bx_u32)order[4 * i] | ((bx_u32)order[4 * i + 1] << 8) | ((bx_u32)order[4 * i + 2] << 16)
                   | ((bx_u32)order[4 * i + 3] << 24);
        if ((bx_u32)a[i] != v)
            bad = 1;
    }
    free(a);
    return same_next(r, &s) | bad;
}

static int run_load_sequence(reader_t *r)
{
    bx_rand_state s;
    bx_u32 seed = rd_u32(r);
    int size = rd_i32(r);
    const unsigned char *order = rd_bytes(r, 40);
    int a[10];
    int i, bad = 0;
    if (order == NULL || size < 0 || size > 10)
        return 1;
    for (i = 0; i < 10; i++)
        a[i] = i;
    bx_srand(&s, seed);
    bx_init_explosions_rand(&s);
    bx_random_shuffle_int(&s, a, a + size);
    for (i = 0; i < 10; i++) {
        bx_u32 v = (bx_u32)order[4 * i] | ((bx_u32)order[4 * i + 1] << 8) | ((bx_u32)order[4 * i + 2] << 16)
                   | ((bx_u32)order[4 * i + 3] << 24);
        if ((bx_u32)a[i] != v)
            bad = 1;
    }
    return same_next(r, &s) | bad;
}

static const entry_t entries[] = {
    {"sine", run_sine, "u32", "i32"},
    {"cosine", run_cosine, "u32", "i32"},
    {"mul_hi", run_mul_hi, "i32,i32", "i32"},
    {"scaled_sine", run_scaled_sine, "u32,i32", "i32"},
    {"scaled_cosine", run_scaled_cosine, "u32,i32", "i32"},
    {"angle_of_vector", run_angle_of_vector, "i32,i32", "i32"},
    {"rotate_point", run_rotate_point, "u32,i32,i32", "i32,i32"},
    {"rotation_matrix", run_rotation_matrix, "u32,u32,u32", "i32,i32,i32,i32,i32,i32,i32,i32,i32"},
    {"apply_rotation", run_apply_rotation, "i32,i32,i32,i32,i32,i32,i32,i32,i32,i32,i32,i32,i32", "i32,i32,i32"},
    {"mul_div", run_mul_div, "i32,i32,i32", "i32"},
    {"sqrt", run_sqrt, "f64", "f64"},
    {"sqrt_float", run_sqrt_float, "u32", "u32"},
    {"ftol", run_ftol, "f64", "i32"},
    {"hypot", run_hypot, "f64,f64", "f64"},
    {"acos", run_acos, "f64", "f64"},
    {"float_text", run_float_text, "str", "u32"},
    {"double_text", run_double_text, "str", "f64"},
    {"rand", run_rand, "u32", "bytes40000"},
    {"shuffle", run_shuffle, "u32,i32", "str,bytes16"},
    {"load_sequence", run_load_sequence, "u32,i32", "bytes40,bytes16"}};

static unsigned char *read_file(const char *dir, const char *name, size_t *size)
{
    char path[1024];
    FILE *f;
    unsigned char *data;
    long n;
    if (strlen(dir) + strlen(name) + 2 > sizeof(path))
        return NULL;
    sprintf(path, "%s/%s", dir, name);
    f = fopen(path, "rb");
    if (f == NULL)
        return NULL;
    fseek(f, 0, SEEK_END);
    n = ftell(f);
    fseek(f, 0, SEEK_SET);
    data = (unsigned char *)malloc((size_t)n + 1);
    if (data != NULL && fread(data, 1, (size_t)n, f) != (size_t)n) {
        free(data);
        data = NULL;
    }
    fclose(f);
    *size = (size_t)n;
    return data;
}

static int run_entry(const char *dir, const entry_t *e)
{
    char file[80];
    unsigned char *data;
    size_t size;
    reader_t r;
    unsigned long cases = 0, mismatches = 0;
    sprintf(file, "%s.bin", e->name);
    data = read_file(dir, file, &size);
    if (data == NULL) {
        printf("%s: cannot read %s/%s\n", e->name, dir, file);
        return 1;
    }
    r.p = data;
    r.end = data + size;
    r.short_read = 0;
    while (r.p < r.end) {
        mismatches += (unsigned long)(e->run(&r) != 0);
        cases++;
    }
    if (r.short_read)
        mismatches++;
    printf("%s: %lu cases, %lu mismatches\n", e->name, cases, mismatches);
    free(data);
    return mismatches != 0;
}

int main(int argc, char **argv)
{
    unsigned char *layouts;
    size_t size, pos = 0;
    unsigned int i;
    int bad = 0;
    unsigned int seen = 0;
    if (argc < 2) {
        printf("usage: vectors_test DIR\n");
        return 2;
    }
    layouts = read_file(argv[1], "layouts.txt", &size);
    if (layouts == NULL) {
        printf("cannot read %s/layouts.txt\n", argv[1]);
        return 2;
    }
    while (pos < size) {
        char line[512];
        char name[80], in[256], out[256];
        size_t n = 0;
        while (pos < size && layouts[pos] != '\n' && n < sizeof(line) - 1)
            line[n++] = (char)layouts[pos++];
        line[n] = 0;
        pos++;
        if (n == 0)
            continue;
        in[0] = out[0] = 0;
        if (sscanf(line, "%79s %255s %255s", name, in, out) < 3) {
            printf("malformed layout line: %s\n", line);
            bad = 1;
            continue;
        }
        for (i = 0; i < sizeof(entries) / sizeof(entries[0]); i++)
            if (strcmp(entries[i].name, name) == 0) {
                seen++;
                if (strcmp(entries[i].in, in) != 0 || strcmp(entries[i].out, out) != 0) {
                    printf("%s: layout %s -> %s is not the one this test reads\n", name, in, out);
                    bad = 1;
                } else {
                    bad |= run_entry(argv[1], &entries[i]);
                }
            }
    }
    if (seen != sizeof(entries) / sizeof(entries[0])) {
        printf("the vectors list %u of the %u library entries\n", seen, (unsigned int)(sizeof(entries) / sizeof(entries[0])));
        bad = 1;
    }
    free(layouts);
    return bad;
}
