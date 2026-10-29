#include "bitexact.h"

#define CASE_COUNT 18

typedef struct {
    bx_u32 h[8];
    unsigned char block[64];
    bx_u32 used;
    bx_u64 total;
} sha256_t;

static const bx_u32 sha256_k[64] = {
    0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u, 0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
    0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u, 0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
    0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu, 0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
    0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u, 0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
    0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u, 0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
    0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u, 0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
    0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u, 0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
    0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u, 0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u};

static bx_u32 rotr(bx_u32 x, int n)
{
    return (x >> n) | (x << (32 - n));
}

static void sha256_init(sha256_t *s)
{
    static const bx_u32 init[8] = {0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
                                   0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u};
    int i;

    for (i = 0; i < 8; i++)
        s->h[i] = init[i];
    s->used = 0;
    s->total = 0;
}

static void sha256_compress(sha256_t *s)
{
    bx_u32 w[64], v[8], t1, t2;
    int i;

    for (i = 0; i < 16; i++)
        w[i] = ((bx_u32)s->block[i * 4] << 24) | ((bx_u32)s->block[i * 4 + 1] << 16) |
               ((bx_u32)s->block[i * 4 + 2] << 8) | (bx_u32)s->block[i * 4 + 3];
    for (i = 16; i < 64; i++) {
        bx_u32 s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
        bx_u32 s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }
    for (i = 0; i < 8; i++)
        v[i] = s->h[i];
    for (i = 0; i < 64; i++) {
        t1 = v[7] + (rotr(v[4], 6) ^ rotr(v[4], 11) ^ rotr(v[4], 25)) + ((v[4] & v[5]) ^ (~v[4] & v[6])) +
             sha256_k[i] + w[i];
        t2 = (rotr(v[0], 2) ^ rotr(v[0], 13) ^ rotr(v[0], 22)) + ((v[0] & v[1]) ^ (v[0] & v[2]) ^ (v[1] & v[2]));
        v[7] = v[6];
        v[6] = v[5];
        v[5] = v[4];
        v[4] = v[3] + t1;
        v[3] = v[2];
        v[2] = v[1];
        v[1] = v[0];
        v[0] = t1 + t2;
    }
    for (i = 0; i < 8; i++)
        s->h[i] += v[i];
}

static void sha256_byte(sha256_t *s, unsigned char b)
{
    s->block[s->used++] = b;
    s->total++;
    if (s->used == 64) {
        sha256_compress(s);
        s->used = 0;
    }
}

static void sha256_final(sha256_t *s, unsigned char out[32])
{
    bx_u64 bits = s->total * 8;
    int i;

    sha256_byte(s, 0x80);
    while (s->used != 56)
        sha256_byte(s, 0);
    for (i = 7; i >= 0; i--)
        sha256_byte(s, (unsigned char)(bits >> (i * 8)));
    for (i = 0; i < 32; i++)
        out[i] = (unsigned char)(s->h[i / 4] >> (24 - (i % 4) * 8));
}

static bx_u32 g_count;

static void put_u32(sha256_t *s, bx_u32 v)
{
    sha256_byte(s, (unsigned char)v);
    sha256_byte(s, (unsigned char)(v >> 8));
    sha256_byte(s, (unsigned char)(v >> 16));
    sha256_byte(s, (unsigned char)(v >> 24));
    g_count++;
}

static void put_u64(sha256_t *s, bx_u64 v)
{
    put_u32(s, (bx_u32)v);
    put_u32(s, (bx_u32)(v >> 32));
}

static bx_u32 float_bits(float f)
{
    union {
        float f;
        bx_u32 u;
    } v;
    v.f = f;
    return v.u;
}

static float float_from(bx_u32 u)
{
    union {
        float f;
        bx_u32 u;
    } v;
    v.u = u;
    return v.f;
}

static bx_u64 double_bits(double d)
{
    union {
        double d;
        bx_u64 u;
    } v;
    v.d = d;
    return v.u;
}

static double double_from(bx_u64 u)
{
    union {
        double d;
        bx_u64 u;
    } v;
    v.u = u;
    return v.d;
}

static bx_u64 lcg(bx_u64 *state)
{
    *state = *state * 6364136223846793005ull + 1442695040888963407ull;
    return *state;
}

static bx_u32 lcg32(bx_u64 *state)
{
    return (bx_u32)(lcg(state) >> 32);
}

#define SCALE_RANDOM 1000

static int make_scales(bx_i32 *scales)
{
    bx_u64 state = 0x5ca1ab1eu;
    int n = 0, k, i;

    scales[n++] = 0;
    scales[n++] = 1;
    scales[n++] = -1;
    scales[n++] = 2147483647;
    scales[n++] = -2147483647 - 1;
    scales[n++] = 2147483646;
    scales[n++] = -2147483647;
    for (k = 1; k < 31; k++) {
        bx_i32 p = (bx_i32)(1u << k);
        scales[n++] = p;
        scales[n++] = -p;
        scales[n++] = p - 1;
        scales[n++] = p + 1;
        scales[n++] = -(p - 1);
        scales[n++] = -(p + 1);
    }
    for (i = 0; i < SCALE_RANDOM; i++)
        scales[n++] = (bx_i32)lcg32(&state);
    for (i = 0; i < 256; i++)
        scales[n++] = (bx_i32)(lcg32(&state) % 2000001u) - 1000000;
    return n;
}

static bx_i32 g_scales[SCALE_RANDOM + 256 + 7 + 6 * 30 + 8];

static void case_scaled_sin(sha256_t *s)
{
    int n = make_scales(g_scales), i;
    bx_i32 angle;

    for (i = 0; i < n; i++)
        for (angle = -32768; angle < 32768; angle++)
            put_u32(s, (bx_u32)bx_ScaledSin((bx_i16)angle, g_scales[i]));
}

static void case_scaled_cos(sha256_t *s)
{
    int n = make_scales(g_scales), i;
    bx_i32 angle;

    for (i = 0; i < n; i++)
        for (angle = -32768; angle < 32768; angle++)
            put_u32(s, (bx_u32)bx_ScaledCos((bx_i16)angle, g_scales[i]));
}

static void case_fixed_sine(sha256_t *s)
{
    bx_u32 angle;

    for (angle = 0; angle < 65536; angle++) {
        put_u32(s, (bx_u32)bx_FixedSine((bx_u16)angle));
        put_u32(s, (bx_u32)bx_fixed_cos((bx_u16)angle));
    }
}

typedef struct {
    char name[32];
    int radius;
    float density;
    float duration;
    float interval;
} MeteorParams;

static MeteorParams *volatile g_params;
static MeteorParams g_params_storage;

#ifdef _MSC_VER
#define NOINLINE __declspec(noinline)
#else
#define NOINLINE __attribute__((noinline))
#endif

NOINLINE static int density_form(const MeteorParams *p)
{
    return (int)(30.0 / p->density);
}

NOINLINE static int duration_form(const MeteorParams *p)
{
    return (int)(p->duration * 30.0);
}

NOINLINE static int interval_form(const MeteorParams *p)
{
    return (int)(p->interval * 30.0);
}

typedef enum { FORM_DENSITY, FORM_DURATION, FORM_INTERVAL } Form;

static double form_value(Form form, float f)
{
    return form == FORM_DENSITY ? 30.0 / f : (double)f * 30.0;
}

static int form_result(Form form, float f)
{
    MeteorParams *p = g_params = &g_params_storage;

    p->density = p->duration = p->interval = f;
    return form == FORM_DENSITY ? density_form(p) : form == FORM_DURATION ? duration_form(p) : interval_form(p);
}

static int in_int_range(double v)
{
    return v < 2147483648.0 && v > -2147483649.0;
}

static void form_range(sha256_t *s, Form form)
{
    bx_u32 bits;

    for (bits = 0x3a800000u; bits <= 0x49800000u; bits++)
        put_u32(s, (bx_u32)form_result(form, float_from(bits)));
}

#define FORM_SAMPLE 4000000

enum { SAMPLE_IN_RANGE, SAMPLE_OVERFLOW, SAMPLE_ALL };

static void form_sample(sha256_t *s, Form form, int mode, bx_u32 sign)
{
    bx_u64 state = 0x0ddba11u;
    bx_u32 bits;
    int i;

    for (i = 0; i < FORM_SAMPLE + 8; i++) {
        float f;
        int r;

        if (i == 0)
            bits = 0;
        else if (i < 8)
            bits = (i == 1) ? 1u : (i == 2) ? 0x007fffffu : (i == 3) ? 0x00800000u : (i == 4) ? 0x7f7fffffu
                 : (i == 5) ? 0x3f800000u : (i == 6) ? 0x5f000000u : 0x4effffffu;
        else
            bits = lcg32(&state) & 0x7fffffffu;
        if (bits >= 0x7f800000u)
            bits -= 0x00800000u;
        f = float_from(bits | sign);
        if (mode != SAMPLE_ALL && in_int_range(form_value(form, f)) == (mode == SAMPLE_OVERFLOW))
            continue;
        r = form_result(form, f);
        put_u32(s, (bx_u32)r);
    }
}

static const bx_u32 g_special_bits[] = {
    0x00000000u, 0x80000000u, 0x7f800000u, 0xff800000u, 0x7fc00000u, 0xffc00000u, 0x7f800001u, 0xff800001u,
    0x7fffffffu, 0xffffffffu, 0x00000001u, 0x80000001u, 0x4f000000u, 0xcf000000u, 0x4f000001u, 0xcf000001u,
    0x7f7fffffu, 0xff7fffffu, 0x322bcc77u, 0x3310fa8au, 0x4cd9cd9cu, 0x4ce4b7a4u};

static void form_special(sha256_t *s, Form form)
{
    unsigned int i;

    for (i = 0; i < sizeof(g_special_bits) / sizeof(g_special_bits[0]); i++)
        put_u32(s, (bx_u32)form_result(form, float_from(g_special_bits[i])));
}

static void case_density_range(sha256_t *s) { form_range(s, FORM_DENSITY); }
static void case_duration_range(sha256_t *s) { form_range(s, FORM_DURATION); }
static void case_interval_range(sha256_t *s) { form_range(s, FORM_INTERVAL); }
static void case_density_sample(sha256_t *s) { form_sample(s, FORM_DENSITY, SAMPLE_IN_RANGE, 0); }
static void case_duration_sample(sha256_t *s) { form_sample(s, FORM_DURATION, SAMPLE_IN_RANGE, 0); }
static void case_density_overflow(sha256_t *s) { form_sample(s, FORM_DENSITY, SAMPLE_OVERFLOW, 0); }
static void case_duration_overflow(sha256_t *s) { form_sample(s, FORM_DURATION, SAMPLE_OVERFLOW, 0); }
static void case_density_negative(sha256_t *s) { form_sample(s, FORM_DENSITY, SAMPLE_ALL, 0x80000000u); }
static void case_duration_negative(sha256_t *s) { form_sample(s, FORM_DURATION, SAMPLE_ALL, 0x80000000u); }
static void case_density_special(sha256_t *s) { form_special(s, FORM_DENSITY); }
static void case_duration_special(sha256_t *s) { form_special(s, FORM_DURATION); }
static void case_interval_special(sha256_t *s) { form_special(s, FORM_INTERVAL); }

typedef struct {
    int windSpeed;
    int windSpeedDenominator;
    float windStrength;
} WindFields;

static WindFields *volatile g_wind;
static WindFields g_wind_storage;

NOINLINE static void wind_strength(WindFields *g)
{
    g->windStrength = (float)g->windSpeed / (float)g->windSpeedDenominator;
    if (1.0 < g->windStrength)
        g->windStrength = 1.0f;
}

static void case_wind(sha256_t *s)
{
    int speed, den;
    WindFields *g = g_wind = &g_wind_storage;

    for (speed = 0; speed <= 10000; speed++)
        for (den = 1; den <= 10000; den++) {
            g->windSpeed = speed;
            g->windSpeedDenominator = den;
            wind_strength(g);
            put_u32(s, float_bits(g->windStrength));
        }
}

NOINLINE static float demote(double d)
{
    return (float)d;
}

#define DEMOTE_RANDOM 4000000

static void case_double_to_float(sha256_t *s)
{
    bx_u64 state = 0xd0b1ed0u;
    bx_u32 i, bits;

    for (i = 0; i < DEMOTE_RANDOM; i++) {
        bx_u64 b = lcg(&state);
        if (((b >> 52) & 0x7ff) == 0x7ff)
            b &= ~(bx_u64)(1ull << 62);
        put_u32(s, float_bits(demote(double_from(b))));
    }
    for (i = 0; i < 1000000; i++)
        put_u32(s, float_bits(demote((double)(int)i / 1000.0)));
    for (i = 0; i < 2000000; i++) {
        bits = lcg32(&state) & 0x7fffffffu;
        if (bits >= 0x7f800000u)
            bits -= 0x00800000u;
        {
            float lo = float_from(bits), hi = float_from(bits + 1);
            double mid = ((double)lo + (double)hi) / 2.0;
            bx_u64 mb = double_bits(mid);

            put_u32(s, float_bits(demote(mid)));
            put_u32(s, float_bits(demote(double_from(mb + 1))));
            put_u32(s, float_bits(demote(double_from(mb - 1))));
            put_u32(s, float_bits(demote(-mid)));
        }
    }
    put_u32(s, float_bits(demote(1.0e39)));
    put_u32(s, float_bits(demote(-1.0e39)));
    put_u32(s, float_bits(demote(1.0e-46)));
    put_u32(s, float_bits(demote(1.0e-45)));
    put_u32(s, float_bits(demote(3.4028235677973366e38)));
}

#define ATOF_TEXTS 300000

static int format_number(char *text, bx_u64 *state)
{
    int n = 0, int_digits = 1 + (int)(lcg32(state) % 3u), frac_digits = (int)(lcg32(state) % 8u), i;

    for (i = 0; i < int_digits; i++)
        text[n++] = (char)('0' + lcg32(state) % 10u);
    if (frac_digits > 0) {
        text[n++] = '.';
        for (i = 0; i < frac_digits; i++)
            text[n++] = (char)('0' + lcg32(state) % 10u);
    }
    text[n] = 0;
    return n;
}

static void case_atof(sha256_t *s)
{
    bx_u64 state = 0xa70f5u;
    char text[32];
    int i;

    for (i = 0; i < ATOF_TEXTS; i++) {
        format_number(text, &state);
        put_u64(s, double_bits(bx_atof_msvc5(text)));
        put_u32(s, float_bits((float)bx_atof_msvc5(text)));
    }
}

typedef struct {
    const char *name;
    void (*run)(sha256_t *);
    int info;
} Case;

static const Case g_cases[CASE_COUNT] = {
    {"scaled_sin", case_scaled_sin, 0},
    {"scaled_cos", case_scaled_cos, 0},
    {"fixed_sine_cos", case_fixed_sine, 0},
    {"meteor_density_range", case_density_range, 0},
    {"meteor_duration_range", case_duration_range, 0},
    {"meteor_interval_range", case_interval_range, 0},
    {"meteor_density_sample", case_density_sample, 0},
    {"meteor_duration_sample", case_duration_sample, 0},
    {"wind_strength", case_wind, 0},
    {"double_to_float", case_double_to_float, 0},
    {"atof_to_float", case_atof, 0},
    {"meteor_density_overflow", case_density_overflow, 0},
    {"meteor_duration_overflow", case_duration_overflow, 0},
    {"meteor_density_negative", case_density_negative, 0},
    {"meteor_duration_negative", case_duration_negative, 0},
    {"meteor_density_special", case_density_special, 0},
    {"meteor_duration_special", case_duration_special, 0},
    {"meteor_interval_special", case_interval_special, 0},
};

static unsigned char g_digest[32];
static bx_u32 g_last_count;

int maths_case_count(void)
{
    return CASE_COUNT;
}

const char *maths_case_name(int index)
{
    return g_cases[index].name;
}

int maths_case_info(int index)
{
    return g_cases[index].info;
}

const unsigned char *maths_run_case(int index)
{
    sha256_t s;

    sha256_init(&s);
    g_count = 0;
    g_cases[index].run(&s);
    g_last_count = g_count;
    sha256_final(&s, g_digest);
    return g_digest;
}

bx_u32 maths_last_count(void)
{
    return g_last_count;
}

#ifdef MATHS_NATIVE
#include <stdio.h>
#include <string.h>
#include <float.h>

int main(int argc, char **argv)
{
    int i, j;

    _control87(0x9001F, 0xFFFFF);
    printf("fpu control word 0x%05X\n", _control87(0, 0));
    for (i = 0; i < maths_case_count(); i++) {
        const unsigned char *d;

        if (argc > 1 && strcmp(argv[1], maths_case_name(i)) != 0)
            continue;
        d = maths_run_case(i);
        printf("%s %u ", maths_case_name(i), maths_last_count());
        for (j = 0; j < 32; j++)
            printf("%02x", d[j]);
        printf("\n");
        fflush(stdout);
    }
    return 0;
}
#endif
