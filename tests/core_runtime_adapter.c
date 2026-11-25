#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../lib/bitexact/bitexact.h"
#include "../lib/bitexact/bx_game.h"
#include "../lib/bitexact/bx_libcmt.h"
#include "../lib/bitexact/bx_stl.h"

typedef struct {
    const unsigned char *p;
    const unsigned char *end;
    int short_read;
} reader_t;

typedef int (*entry_fn)(reader_t *r);

static FILE *out;

static int (*volatile call_atoi)(const char *) = atoi;
static int (*volatile call_strncmp)(const char *, const char *, size_t) = strncmp;
static char *(*volatile call_strncpy)(char *, const char *, size_t) = strncpy;
static char *(*volatile call_strstr)(const char *, const char *) = strstr;
static char *(*volatile call_strchr)(const char *, int) = strchr;
static char *(*volatile call_strrchr)(const char *, int) = strrchr;
static size_t (*volatile call_strcspn)(const char *, const char *) = strcspn;
static void *(*volatile call_memmove)(void *, const void *, size_t) = memmove;

extern int _ftol(void);

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

static void wr_u32(bx_u32 v)
{
    unsigned char b[4];
    b[0] = (unsigned char)v;
    b[1] = (unsigned char)(v >> 8);
    b[2] = (unsigned char)(v >> 16);
    b[3] = (unsigned char)(v >> 24);
    fwrite(b, 1, 4, out);
}

static void wr_i32(int v)
{
    wr_u32((bx_u32)v);
}

static void wr_f64(double d)
{
    union {
        double d;
        bx_u32 w[2];
    } b;
    b.d = d;
    wr_u32(b.w[0]);
    wr_u32(b.w[1]);
}

static void wr_bytes(const void *p, size_t n)
{
    fwrite(p, 1, n, out);
}

static void wr_str(const char *p, size_t n)
{
    wr_u32((bx_u32)n);
    wr_bytes(p, n);
}

static bx_u32 float_bits(float f)
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

static int e_sine(reader_t *r)
{
    wr_i32(bx_FixedSine((bx_u16)rd_u32(r)));
    return 0;
}

static int e_cosine(reader_t *r)
{
    wr_i32(bx_fixed_cos((bx_u16)rd_u32(r)));
    return 0;
}

static int e_mul_hi(reader_t *r)
{
    int a = rd_i32(r);
    int b = rd_i32(r);
    wr_i32(bx_mul_hi32(a, b));
    return 0;
}

static int e_scaled_sine(reader_t *r)
{
    bx_u32 angle = rd_u32(r);
    int scale = rd_i32(r);
    wr_i32(bx_ScaledSin((bx_i16)angle, scale));
    return 0;
}

static int e_scaled_cosine(reader_t *r)
{
    bx_u32 angle = rd_u32(r);
    int scale = rd_i32(r);
    wr_i32(bx_ScaledCos((bx_i16)angle, scale));
    return 0;
}

static int e_angle_of_vector(reader_t *r)
{
    int x = rd_i32(r);
    int z = rd_i32(r);
    wr_i32(bx_AngleOfVector(x, z));
    return 0;
}

static int e_rotate_point(reader_t *r)
{
    bx_u32 angle = rd_u32(r);
    int xz[2];
    xz[0] = rd_i32(r);
    xz[1] = rd_i32(r);
    bx_RotatePoint2D((bx_i16)angle, xz);
    wr_i32(xz[0]);
    wr_i32(xz[1]);
    return 0;
}

static int e_rotation_matrix(reader_t *r)
{
    bx_i16 angles[3];
    bx_i32 m[9];
    int i;
    for (i = 0; i < 3; i++)
        angles[i] = (bx_i16)rd_u32(r);
    bx_build_rotation_matrix(angles, m);
    for (i = 0; i < 9; i++)
        wr_i32(m[i]);
    return 0;
}

static int e_apply_rotation(reader_t *r)
{
    bx_i32 m[9];
    bx_i32 in[3];
    bx_i32 res[3];
    int aliased, i;
    for (i = 0; i < 9; i++)
        m[i] = rd_i32(r);
    for (i = 0; i < 3; i++)
        in[i] = rd_i32(r);
    aliased = rd_i32(r);
    if (aliased) {
        bx_apply_rotation_matrix(m, in, in);
        memcpy(res, in, sizeof(res));
    } else {
        bx_apply_rotation_matrix(m, in, res);
    }
    for (i = 0; i < 3; i++)
        wr_i32(res[i]);
    return 0;
}

static int e_mul_div(reader_t *r)
{
    int a = rd_i32(r);
    int b = rd_i32(r);
    int c = rd_i32(r);
    wr_i32(bx_MulDivTrunc(a, b, c));
    return 0;
}

static int e_sqrt(reader_t *r)
{
    wr_f64(bx_sqrt(rd_f64(r)));
    return 0;
}

static int e_sqrt_float(reader_t *r)
{
    float f = float_from_bits(rd_u32(r));
    wr_u32(float_bits((float)bx_sqrt((double)f)));
    return 0;
}

static int e_hypot(reader_t *r)
{
    double x = rd_f64(r);
    double y = rd_f64(r);
    wr_f64(bx_hypot(x, y));
    return 0;
}

static int e_acos(reader_t *r)
{
    wr_f64(bx_acos(rd_f64(r)));
    return 0;
}

static int e_float_text(reader_t *r)
{
    char *text = rd_text(r);
    if (text == NULL)
        return 1;
    wr_u32(float_bits(bx_strtof_msvc5(text, 0)));
    free(text);
    return 0;
}

static int e_double_text(reader_t *r)
{
    char *text = rd_text(r);
    if (text == NULL)
        return 1;
    wr_f64(bx_atof_msvc5(text));
    free(text);
    return 0;
}

static void wr_next(bx_rand_state *s)
{
    int k;
    for (k = 0; k < 4; k++)
        wr_i32(bx_rand(s));
}

static int e_rand(reader_t *r)
{
    bx_rand_state s;
    int i;
    bx_srand(&s, rd_u32(r));
    for (i = 0; i < 10000; i++)
        wr_i32(bx_rand(&s));
    return 0;
}

static int e_shuffle(reader_t *r)
{
    bx_rand_state s;
    bx_u32 seed = rd_u32(r);
    int size = rd_i32(r);
    int *a;
    int i;
    if (size < 0 || size > (1 << 24))
        return 1;
    a = (int *)malloc((size_t)size * sizeof(int) + sizeof(int));
    if (a == NULL)
        return 1;
    for (i = 0; i < size; i++)
        a[i] = i * 3 + 1;
    bx_srand(&s, seed);
    bx_random_shuffle_int(&s, a, a + size);
    wr_u32((bx_u32)size * 4);
    for (i = 0; i < size; i++)
        wr_i32(a[i]);
    free(a);
    wr_next(&s);
    return 0;
}

static int e_load_sequence(reader_t *r)
{
    bx_rand_state s;
    bx_u32 seed = rd_u32(r);
    int size = rd_i32(r);
    int a[10];
    int i;
    if (size < 0 || size > 10)
        return 1;
    for (i = 0; i < 10; i++)
        a[i] = i;
    bx_srand(&s, seed);
    bx_init_explosions_rand(&s);
    bx_random_shuffle_int(&s, a, a + size);
    for (i = 0; i < 10; i++)
        wr_i32(a[i]);
    wr_next(&s);
    return 0;
}

static int e_atoi(reader_t *r)
{
    char *s = rd_text(r);
    if (s == NULL)
        return 1;
    wr_i32(call_atoi(s));
    free(s);
    return 0;
}

static int e_itoa(reader_t *r)
{
    char buf[40];
    int v = rd_i32(r);
    int radix = rd_i32(r);
    memset(buf, 0xAA, sizeof(buf));
    _itoa(v, buf, radix);
    buf[39] = 0;
    wr_str(buf, strlen(buf));
    return 0;
}

static int e_strcmpi(reader_t *r)
{
    char *a = rd_text(r);
    char *b = rd_text(r);
    if (a == NULL || b == NULL)
        return 1;
    wr_i32(_strcmpi(a, b));
    free(a);
    free(b);
    return 0;
}

static int e_strnicmp(reader_t *r)
{
    char *a = rd_text(r);
    char *b = rd_text(r);
    int n = rd_i32(r);
    if (a == NULL || b == NULL)
        return 1;
    wr_i32(_strnicmp(a, b, (size_t)n));
    free(a);
    free(b);
    return 0;
}

static int e_strncmp(reader_t *r)
{
    char *a = rd_text(r);
    char *b = rd_text(r);
    int n = rd_i32(r);
    if (a == NULL || b == NULL)
        return 1;
    wr_i32(call_strncmp(a, b, (size_t)n));
    free(a);
    free(b);
    return 0;
}

static int e_compare_bytes(reader_t *r)
{
    static signed char res[5][256];
    char a[2], b[2];
    int j, n;
    a[0] = (char)rd_i32(r);
    a[1] = b[1] = 0;
    for (j = 0; j < 256; j++) {
        b[0] = (char)j;
        res[0][j] = (signed char)_strcmpi(a, b);
        for (n = 1; n <= 2; n++) {
            res[n][j] = (signed char)_strnicmp(a, b, (size_t)n);
            res[2 + n][j] = (signed char)call_strncmp(a, b, (size_t)n);
        }
    }
    wr_bytes(res, sizeof(res));
    return 0;
}

static int e_strlwr(reader_t *r)
{
    char *s = rd_text(r);
    if (s == NULL)
        return 1;
    _strlwr(s);
    wr_str(s, strlen(s));
    free(s);
    return 0;
}

static int e_tolower(reader_t *r)
{
    int (*volatile fn)(int) = tolower;
    wr_i32(fn(rd_i32(r)));
    return 0;
}

static int e_toupper(reader_t *r)
{
    int (*volatile fn)(int) = toupper;
    wr_i32(fn(rd_i32(r)));
    return 0;
}

static int e_isspace(reader_t *r)
{
    int (*volatile fn)(int) = isspace;
    wr_i32(fn(rd_i32(r)));
    return 0;
}

static int e_isdigit(reader_t *r)
{
    int (*volatile fn)(int) = isdigit;
    wr_i32(fn(rd_i32(r)));
    return 0;
}

static int e_isalnum(reader_t *r)
{
    int (*volatile fn)(int) = isalnum;
    wr_i32(fn(rd_i32(r)));
    return 0;
}

static int e_strncpy(reader_t *r)
{
    char dst[24];
    char *src = rd_text(r);
    int n = rd_i32(r);
    if (src == NULL)
        return 1;
    memset(dst, 0xAA, sizeof(dst));
    call_strncpy(dst, src, (size_t)n);
    wr_bytes(dst, sizeof(dst));
    free(src);
    return 0;
}

static int offset_of(const char *base, const char *p)
{
    return p == NULL ? -1 : (int)(p - base);
}

static int e_strstr(reader_t *r)
{
    char *hay = rd_text(r);
    char *needle = rd_text(r);
    if (hay == NULL || needle == NULL)
        return 1;
    wr_i32(offset_of(hay, call_strstr(hay, needle)));
    free(hay);
    free(needle);
    return 0;
}

static int e_strchr(reader_t *r)
{
    char *hay = rd_text(r);
    int c = rd_i32(r);
    if (hay == NULL)
        return 1;
    wr_i32(offset_of(hay, call_strchr(hay, c)));
    free(hay);
    return 0;
}

static int e_strrchr(reader_t *r)
{
    char *hay = rd_text(r);
    int c = rd_i32(r);
    if (hay == NULL)
        return 1;
    wr_i32(offset_of(hay, call_strrchr(hay, c)));
    free(hay);
    return 0;
}

static int e_strcspn(reader_t *r)
{
    char *hay = rd_text(r);
    char *set = rd_text(r);
    if (hay == NULL || set == NULL)
        return 1;
    wr_i32((int)call_strcspn(hay, set));
    free(hay);
    free(set);
    return 0;
}

#define MAXTOK 24

static int e_strtok(reader_t *r)
{
    char buf[48];
    int toks[MAXTOK * 2];
    int n = 0;
    char *t;
    char *s = rd_text(r);
    char *d1 = rd_text(r);
    char *d2 = rd_text(r);
    if (s == NULL || d1 == NULL || d2 == NULL || strlen(s) >= sizeof(buf))
        return 1;
    memset(buf, 0xAA, sizeof(buf));
    strcpy(buf, s);
    memset(toks, 0, sizeof(toks));
    t = strtok(buf, d1);
    while (t != NULL && n < MAXTOK - 2) {
        toks[2 * n] = (int)(t - buf);
        toks[2 * n + 1] = (int)strlen(t);
        n++;
        t = strtok(NULL, d2);
    }
    toks[2 * n] = t == NULL ? -1 : -2;
    t = strtok(NULL, d2);
    toks[2 * n + 2] = t == NULL ? -1 : (int)(t - buf);
    wr_i32(n);
    wr_bytes(toks, sizeof(toks));
    wr_bytes(buf, sizeof(buf));
    free(s);
    free(d1);
    free(d2);
    return 0;
}

static int e_memmove(reader_t *r)
{
    unsigned char buf[64];
    unsigned char *res;
    int i;
    int d = rd_i32(r);
    int s = rd_i32(r);
    int n = rd_i32(r);
    if (d < 0 || s < 0 || n < 0 || d + n > 64 || s + n > 64)
        return 1;
    for (i = 0; i < 64; i++)
        buf[i] = (unsigned char)(i * 37 + 11);
    res = (unsigned char *)call_memmove(buf + d, buf + s, (size_t)n);
    wr_i32((int)(res - buf));
    wr_bytes(buf, sizeof(buf));
    return 0;
}

#define MAXCONV 8

static int e_sprintf(reader_t *r)
{
    static char buf[8192];
    unsigned long w[20];
    char *fmt = rd_text(r);
    char *kinds = rd_text(r);
    size_t alen = rd_u32(r);
    const unsigned char *a = rd_bytes(r, alen);
    const unsigned char *p = a;
    const unsigned char *aend = a + alen;
    char *strs[MAXCONV];
    int nstr = 0, np = 0, ret;
    size_t j, nk;
    if (fmt == NULL || kinds == NULL || (a == NULL && alen != 0) || strlen(fmt) >= 256)
        return 1;
    nk = strlen(kinds);
    if (nk > MAXCONV)
        return 1;
    memset(w, 0, sizeof(w));
    for (j = 0; j < nk; j++) {
        if (kinds[j] == 'i') {
            if (aend - p < 4)
                return 1;
            w[np++] = (unsigned long)p[0] | ((unsigned long)p[1] << 8) | ((unsigned long)p[2] << 16)
                      | ((unsigned long)p[3] << 24);
            p += 4;
        } else if (kinds[j] == 'f') {
            if (aend - p < 8)
                return 1;
            memcpy(&w[np], p, 8);
            np += 2;
            p += 8;
        } else if (kinds[j] == 's') {
            size_t n;
            char *s;
            if (aend - p < 4)
                return 1;
            n = (size_t)p[0] | ((size_t)p[1] << 8) | ((size_t)p[2] << 16) | ((size_t)p[3] << 24);
            p += 4;
            if ((size_t)(aend - p) < n)
                return 1;
            s = (char *)malloc(n + 1);
            if (s == NULL)
                return 1;
            memcpy(s, p, n);
            s[n] = 0;
            p += n;
            strs[nstr++] = s;
            w[np++] = (unsigned long)s;
        } else {
            return 1;
        }
    }
    memset(buf, 0xAA, sizeof(buf));
    ret = sprintf(buf, fmt, w[0], w[1], w[2], w[3], w[4], w[5], w[6], w[7], w[8], w[9], w[10], w[11], w[12], w[13],
                  w[14], w[15]);
    wr_str(buf, ret < 0 ? 0 : (size_t)ret);
    wr_i32(ret);
    while (nstr > 0)
        free(strs[--nstr]);
    free(fmt);
    free(kinds);
    return 0;
}

static int e_sscanf(reader_t *r)
{
    char fcopy[128];
    char tcopy[128];
    int ints[4];
    char sb[64];
    void *ptr[6];
    int ni, np = 0, ret;
    const char *f;
    char *fmt = rd_text(r);
    char *text = rd_text(r);
    if (fmt == NULL || text == NULL || strlen(fmt) >= sizeof(fcopy) || strlen(text) >= sizeof(tcopy))
        return 1;
    for (ni = 0; ni < 4; ni++)
        ints[ni] = 0x5a5a5a5a;
    memset(sb, 0xAA, sizeof(sb));
    ni = 0;
    for (f = fmt; *f; f++) {
        if (*f != '%')
            continue;
        f++;
        if (*f == '%')
            continue;
        while (*f >= '0' && *f <= '9')
            f++;
        if (*f == '[') {
            f++;
            if (*f == '^')
                f++;
            if (*f == ']')
                f++;
            while (*f && *f != ']')
                f++;
            ptr[np++] = sb;
        } else if (*f == 's') {
            ptr[np++] = sb;
        } else {
            ptr[np++] = &ints[ni++];
        }
        if (*f == 0)
            break;
    }
    while (np < 6)
        ptr[np++] = NULL;
    strcpy(fcopy, fmt);
    strcpy(tcopy, text);
    ret = sscanf(tcopy, fcopy, ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5]);
    wr_i32(ret);
    wr_bytes(ints, sizeof(ints));
    wr_bytes(sb, sizeof(sb));
    free(fmt);
    free(text);
    return 0;
}

static int e_ftol(reader_t *r)
{
    double x = rd_f64(r);
    int v;
    __asm {
        fld qword ptr x
        call _ftol
        mov v, eax
    }
    wr_i32(v);
    return 0;
}

static const struct {
    const char *name;
    entry_fn fn;
} entries[] = {
    {"sine", e_sine},
    {"cosine", e_cosine},
    {"mul_hi", e_mul_hi},
    {"scaled_sine", e_scaled_sine},
    {"scaled_cosine", e_scaled_cosine},
    {"angle_of_vector", e_angle_of_vector},
    {"rotate_point", e_rotate_point},
    {"rotation_matrix", e_rotation_matrix},
    {"apply_rotation", e_apply_rotation},
    {"mul_div", e_mul_div},
    {"sqrt", e_sqrt},
    {"sqrt_float", e_sqrt_float},
    {"hypot", e_hypot},
    {"acos", e_acos},
    {"float_text", e_float_text},
    {"double_text", e_double_text},
    {"rand", e_rand},
    {"shuffle", e_shuffle},
    {"load_sequence", e_load_sequence},
    {"atoi", e_atoi},
    {"itoa", e_itoa},
    {"strcmpi", e_strcmpi},
    {"strnicmp", e_strnicmp},
    {"strncmp", e_strncmp},
    {"compare_bytes", e_compare_bytes},
    {"strlwr", e_strlwr},
    {"tolower", e_tolower},
    {"toupper", e_toupper},
    {"isspace", e_isspace},
    {"isdigit", e_isdigit},
    {"isalnum", e_isalnum},
    {"strncpy", e_strncpy},
    {"strstr", e_strstr},
    {"strchr", e_strchr},
    {"strrchr", e_strrchr},
    {"strcspn", e_strcspn},
    {"strtok", e_strtok},
    {"memmove", e_memmove},
    {"sprintf", e_sprintf},
    {"sscanf", e_sscanf},
    {"ftol", e_ftol}};

int main(int argc, char **argv)
{
    unsigned int i;
    entry_fn fn = NULL;
    FILE *f;
    unsigned char *data;
    long n;
    reader_t r;
    if (argc != 4)
        return 2;
    for (i = 0; i < sizeof(entries) / sizeof(entries[0]); i++)
        if (strcmp(entries[i].name, argv[1]) == 0)
            fn = entries[i].fn;
    if (fn == NULL)
        return 2;
    f = fopen(argv[2], "rb");
    if (f == NULL)
        return 2;
    fseek(f, 0, SEEK_END);
    n = ftell(f);
    fseek(f, 0, SEEK_SET);
    data = (unsigned char *)malloc((size_t)n + 1);
    if (data == NULL || fread(data, 1, (size_t)n, f) != (size_t)n)
        return 2;
    fclose(f);
    out = fopen(argv[3], "wb");
    if (out == NULL)
        return 2;
    r.p = data;
    r.end = data + n;
    r.short_read = 0;
    while (r.p < r.end) {
        if (fn(&r) != 0 || r.short_read) {
            fclose(out);
            return 2;
        }
    }
    fclose(out);
    return 0;
}
