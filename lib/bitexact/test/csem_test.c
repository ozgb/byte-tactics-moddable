#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../bitexact.h"
#include "kit.h"
#include "vectors.h"

#define MAXCONV 8
#define MAXTOK 24
#define STR_BYTES 64

static const char *const sprintf_formats[] = {
    " - %3d : %3d : %3d = %3d - '%s\t\t:%s'\r\n",
    " - name = '%s' from file %s",
    "base name '%s'\n",
    "name = '%s'\n",
    " (%+d)",
    " +%d",
    " - %s\n",
    " E:%d",
    " M:%d",
    " to %i",
    "%+d",
    "+%d",
    "%.1f",
    "%02d:%02d:%02d",
    "%02x%c",
    "%08lX%c",
    "%08lX",
    "%08lX: ",
    "%d %s - %s",
    "%d %s",
    "%d MBytes physical memory\n",
    "%d processors\n",
    "%d",
    "%s  %c%d\n",
    "%s  M:%d E:%d",
    "%s %s%s",
    "%s %s",
    "%s : %02d:%02d:%02d",
    "%s : %d  (Max %d)",
    "%s caused an %s in\n",
    "%s%d",
    "%s%s%s%04i.%s",
    "%s%s%s*.%s",
    "%s%s",
    "%s, run by %s on %s\n",
    "%s-%s\\%s",
    "%s0",
    "%s: %s",
    "%sMAIN2.GUI",
    "%s\\%s",
    "%s\\*",
    "%s\\screenshots",
    "+%.0f",
    "+%.1f",
    "-%.0f",
    "-%.1f",
    "<%s%s%s> %s",
    "Access violation: Illegal %s, data address 0x%08lX\n",
    "CANBUILD %s",
    "CLASS%d",
    "ContextFlags = %08lX\n",
    "Control Word = %08lX\t\t",
    "Cr0NpxState = %08lX\n",
    "DELTATIME: %d\n",
    "DataOffset = %08lX\n",
    "DataSelector = %08lX\t\t",
    "Dr0 = %08lX\t",
    "Dr1 = %08lX\t",
    "Dr2 = %08lX\n",
    "Dr3 = %08lX\t",
    "Dr6 = %08lX\t",
    "Dr7 = %08lX\n",
    "EAX=%08lX CS=%04lX EIP=%08lX EFLGS=%08lX\n",
    "EBX=%08lX SS=%04lX ESP=%08lX EBP=%08lX\n",
    "ECX=%08lX DS=%04lX ESI=%08lX FS=%08lX\n",
    "EDX=%08lX ES=%04lX EDI=%08lX GS=%08lX\n",
    "Error: Could not find start position number %i on the map!",
    "ErrorOffset = %08lX\n",
    "ErrorSelector = %08lX\t",
    "Exception handler called in %s. ",
    "ExceptionAddress = %08lX\n",
    "ExceptionCode = %08lX",
    "ExceptionFlags = %08lX\t",
    "Executable is %d bytes long and dated %d/%d/%d %02d:%02d:%02d\n",
    "FRATE: %d\n",
    "GAMETIME: %d\n",
    "Hey!  Somebody forgot to set downloadable=1 for %s",
    "Hey, joker!  Mission file %s is corrupt (no header found).",
    "Hey, joker!  There is no mission defintion for this mission: %s",
    "Instruction pointer is %08lX\n",
    "LIVEALLY%d",
    "Library version %d. Library date %s\n",
    "MISSION%d",
    "MODE %s INFO %s",
    "MOVEORD: %d FIREORD: %d\n",
    "OK.  Will share energy if above %d",
    "OK.  Will share metal if above %d",
    "Ok.  Outgoing packet compression turned %s",
    "PACKETS: %d %d %d\n",
    "PFSTATE %d, PFABLE %d\n",
    "Player number %d invalid for unit %s",
    "Player%i",
    "Receive - %1.1f K/s",
    "Record \"%s\" missing from feature files",
    "Schema %i",
    "Script%i",
    "Send - %1.1f K/s",
    "Stack goes from %08lX to %08lX\n",
    "StatusWord = %08lX\n",
    "TABLE%d",
    "TagWord = %08lX\t\t",
    "Time: %s",
    "Toggled ShareEnergy to: %s",
    "Toggled ShareMapping to: %s",
    "Toggled ShareMetal to: %s",
    "Toggled ShareRadar to: %s",
    "UNITS %d\\%d\n",
    "UTC link time: %08lx - %s",
    "UTYPENAME%4d",
    "Unknown TNT version:  0x%08x",
    "X: %d  Y: %d\n",
    "XYH: %d %d %d\n",
    "[HAPI_readfromfile] Decompression Error: %s\n",
    "block %d of %d\n",
    "canbuild%d",
    "debugdat\\%s.txt",
    "length = %d\n",
    "line%d",
    "module %s at %04x:%08lx.\n",
    "netCondenser: BUFFER_ACC_SIZE = %ld, requested send size = %ld , resized buffer to accomodate.\n",
    "netCondenser[1]: r_unpackedSize = %ld, client buffersize = %ld\n",
    "netCondenser[2]: r_unpackedSize = %ld, client buffersize = %ld\n",
    "netCondenser[3]: needed %ld, had client buffersize = %ld\n",
    "pS=%4d pR=%4d (S=%d/%4d, R=%d/%4d) C=%3d%%\n",
    "play cdaudio from %i",
    "savegame\\%s.sav",
    "stagebuttn%d",
    "u%04xacc",
    "u%04xm%04x",
    "u%04xmob",
    "weapon %d - area of effect",
    "weapon %d - coverage",
    "will be rejected in %d seconds"
};

static const char *const sscanf_formats[] = {
    " %[a-zA-Z0-9.]",
    " %[a-zA-Z0-9_.]",
    " %d %d",
    " %d",
    " %s %n",
    "%[a-zA-Z],%i",
    "%[a-zA-Z],%i,%i,%i",
    "%d",
    "%x"
};

typedef struct {
    const char *name;
    unsigned long cases;
    unsigned long edge;
    unsigned long mismatches;
    unsigned long skipped;
} stat_t;

static stat_t st_atoi = {"atoi"};
static stat_t st_itoa = {"itoa"};
static stat_t st_strcmpi = {"strcmpi"};
static stat_t st_strnicmp = {"strnicmp"};
static stat_t st_strncmp = {"strncmp"};
static stat_t st_strlwr = {"strlwr"};
static stat_t st_tolower = {"tolower"};
static stat_t st_toupper = {"toupper"};
static stat_t st_isspace = {"isspace"};
static stat_t st_isdigit = {"isdigit"};
static stat_t st_isalnum = {"isalnum"};
static stat_t st_strncpy = {"strncpy"};
static stat_t st_strstr = {"strstr"};
static stat_t st_strchr = {"strchr"};
static stat_t st_strrchr = {"strrchr"};
static stat_t st_strcspn = {"strcspn"};
static stat_t st_strtok = {"strtok"};
static stat_t st_memmove = {"memmove"};
static stat_t st_sprintf = {"sprintf"};
static stat_t st_sscanf = {"sscanf"};
static stat_t st_ftol = {"ftol"};
static stat_t *all_stats[] = {&st_atoi, &st_itoa, &st_strcmpi, &st_strnicmp, &st_strncmp, &st_strlwr, &st_tolower,
                              &st_toupper, &st_isspace, &st_isdigit, &st_isalnum, &st_strncpy, &st_strstr,
                              &st_strchr, &st_strrchr, &st_strcspn, &st_strtok, &st_memmove, &st_sprintf,
                              &st_sscanf, &st_ftol};

static int (*volatile call_atoi)(const char *) = atoi;
static int (*volatile call_strncmp)(const char *, const char *, size_t) = strncmp;
static char *(*volatile call_strncpy)(char *, const char *, size_t) = strncpy;
static char *(*volatile call_strstr)(const char *, const char *) = strstr;
static char *(*volatile call_strchr)(const char *, int) = strchr;
static char *(*volatile call_strrchr)(const char *, int) = strrchr;
static size_t (*volatile call_strcspn)(const char *, const char *) = strcspn;
static void *(*volatile call_memmove)(void *, const void *, size_t) = memmove;

static unsigned int rng_state = 0x6d2b79f5u;

static unsigned int rnd32(void)
{
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return rng_state;
}

static int rndn(int n)
{
    return (int)(rnd32() % (unsigned int)n);
}

static stat_t *cur;
static int cur_bad;
static int kit_quiet;
static int pack_bad;
static const char *cur_note;
static unsigned char rtmp[8192];

static void begin(stat_t *s, int edge)
{
    cur = s;
    cur_bad = 0;
    cur_note = NULL;
    s->cases++;
    if (edge)
        s->edge++;
    if (!kit_quiet)
        kit_begin(s->name, edge);
}

static void finish(void)
{
    kit_end();
    if (cur_bad) {
        cur->mismatches++;
        if (cur->mismatches <= 5)
            printf("  %s mismatch at case %lu%s%s\n", cur->name, cur->cases, cur_note ? " format " : "",
                   cur_note ? cur_note : "");
    }
}

static int out_i(int v)
{
#ifdef BX_REPLAY
    int w = (int)vec_get();
    if (w != v)
        cur_bad = 1;
    kit_i32(w);
    return w;
#else
    vec_put((bx_u32)v);
    return v;
#endif
}

static void out_bytes(const void *p, int n)
{
#ifdef BX_REPLAY
    vec_get_bytes(rtmp, n);
    if (memcmp(rtmp, p, n) != 0)
        cur_bad = 1;
    kit_bytes(rtmp, n);
#else
    vec_put_bytes(p, n);
#endif
}

static void out_str_n(const char *p, int n)
{
#ifdef BX_REPLAY
    int w = (int)vec_get();
    if (w < 0 || w > (int)sizeof(rtmp)) {
        cur_bad = 1;
        return;
    }
    vec_get_bytes(rtmp, w);
    if (w != n || memcmp(rtmp, p, n) != 0)
        cur_bad = 1;
    kit_u32((bx_u32)w);
    kit_bytes(rtmp, w);
#else
    vec_put((bx_u32)n);
    vec_put_bytes(p, n);
#endif
}

static void out_str(const char *p)
{
    out_str_n(p, (int)strlen(p));
}

static int rchar(void)
{
    int r = rndn(10);
    if (r < 4 || r == 9)
        return rndn(2) ? 'a' + rndn(26) : 'A' + rndn(26);
    if (r == 4)
        return '0' + rndn(10);
    if (r == 5)
        return "[\\]^_`"[rndn(6)];
    if (r == 6)
        return rndn(2) ? ' ' : '.';
    return 0x80 + rndn(128);
}

static int rbyte(void)
{
    return 1 + rndn(255);
}

static void test_atoi_case(const char *s, int edge)
{
    begin(&st_atoi, edge);
    kit_str(s);
    out_i(call_atoi(s));
    finish();
}

static void test_atoi(void)
{
    static const char *const ws[] = {"", " ", "\t", "\n", "\v", "\f", "\r", "  ", " \t\n\v\f\r", "\034", "\240", "\377"};
    static const char *const signs[] = {"", "+", "-", "+-", "--", "- ", "+ "};
    static const char *const bodies[] = {
        "", "0", "7", "007", "-0", "12ab", "2147483647", "2147483648", "2147483649", "4294967295", "4294967296",
        "4294967297", "4294967298", "9999999999", "99999999999999999999", "0x10", "1e5", ".5", "5.", " 5", "1 2",
        "1\n", "12\240", "\240", "abc", "123456789012", "00000000000000000000001"};
    char buf[160];
    int a, b, c, i, j, k;
    kit_layout("atoi", "in s str\nout v i32\n");
    for (a = 0; a < (int)(sizeof(ws) / sizeof(ws[0])); a++)
        for (b = 0; b < (int)(sizeof(signs) / sizeof(signs[0])); b++)
            for (c = 0; c < (int)(sizeof(bodies) / sizeof(bodies[0])); c++) {
                strcpy(buf, ws[a]);
                strcat(buf, signs[b]);
                strcat(buf, bodies[c]);
                test_atoi_case(buf, 1);
            }
    for (k = 0; k < 2; k++)
        for (i = 0; i < 24; i++) {
            sprintf(buf, "%s%lu", k ? "-" : "", 2147483640ul + (unsigned long)i);
            test_atoi_case(buf, 1);
            sprintf(buf, "%s42949672%02d", k ? "-" : "", 90 + (i % 10));
            test_atoi_case(buf, 1);
            sprintf(buf, "%s42949673%02d", k ? "-" : "", i);
            test_atoi_case(buf, 1);
            sprintf(buf, "%s2147483%03d", k ? "-" : "", 640 + i);
            test_atoi_case(buf, 1);
        }
    for (i = 0; i < 6000; i++) {
        int n = rndn(4);
        buf[0] = 0;
        for (j = 0; j < n; j++) {
            int len = (int)strlen(buf);
            buf[len] = rndn(3) ? " \t\n\v\f\r"[rndn(6)] : (char)rbyte();
            buf[len + 1] = 0;
        }
        if (rndn(3))
            strcat(buf, rndn(3) ? (rndn(2) ? "+" : "-") : (rndn(2) ? "-+" : "+-"));
        n = rndn(15);
        for (j = 0; j < n; j++) {
            int len = (int)strlen(buf);
            buf[len] = (char)('0' + (rndn(4) == 0 ? 9 : rndn(10)));
            buf[len + 1] = 0;
        }
        n = rndn(4);
        for (j = 0; j < n; j++) {
            int len = (int)strlen(buf);
            buf[len] = rndn(2) ? (char)rbyte() : (char)rchar();
            buf[len + 1] = 0;
        }
        test_atoi_case(buf, 0);
    }
}

static void test_itoa_case(int v, int radix, int edge)
{
    char buf[40];
    memset(buf, 0xAA, sizeof(buf));
    begin(&st_itoa, edge);
    kit_i32(v);
    kit_i32(radix);
    _itoa(v, buf, radix);
    buf[39] = 0;
    out_str(buf);
    finish();
}

static void test_itoa(void)
{
    static const int small[] = {0, 1, -1, 10, 255, 256, INT_MIN, INT_MAX, 0x12345678, -0x12345678, 65535, -65536,
                                -2, 15, 16, 17, 0x7fff, 0x8000, INT_MIN + 1, 0xdeadbeef};
    int i, k, p;
    kit_layout("itoa", "in value i32\nin radix i32\nout s str\n");
    for (i = -300; i <= 300; i++)
        test_itoa_case(i, 10, 1);
    test_itoa_case(INT_MAX, 10, 1);
    test_itoa_case(INT_MAX - 1, 10, 1);
    test_itoa_case(INT_MIN, 10, 1);
    test_itoa_case(INT_MIN + 1, 10, 1);
    test_itoa_case(0x7fff, 10, 1);
    test_itoa_case(0x8000, 10, 1);
    test_itoa_case(0xffff, 10, 1);
    test_itoa_case(0x10000, 10, 1);
    test_itoa_case(-0x8000, 10, 1);
    test_itoa_case(-0x8001, 10, 1);
    for (p = 1, k = 0; k < 10; k++, p *= 10) {
        test_itoa_case(p, 10, 1);
        test_itoa_case(p - 1, 10, 1);
        test_itoa_case(p + 1, 10, 1);
        test_itoa_case(-p, 10, 1);
        test_itoa_case(-(p - 1), 10, 1);
        test_itoa_case(-(p + 1), 10, 1);
    }
    for (p = 1, k = 0; k < 8; k++, p *= 16) {
        test_itoa_case(p, 10, 1);
        test_itoa_case(p - 1, 10, 1);
        test_itoa_case(-p, 10, 1);
    }
    for (i = 0; i < (int)(sizeof(small) / sizeof(small[0])); i++) {
        test_itoa_case(small[i], 16, 1);
        test_itoa_case(small[i], 2, 1);
        test_itoa_case(small[i], 8, 1);
    }
    for (i = 0; i < 8000; i++) {
        int v = (int)rnd32();
        v >>= rndn(32);
        test_itoa_case(rndn(2) ? v : -v, 10, 0);
    }
}

static int test_cmp_case(int kind, const char *a, const char *b, int n, int edge)
{
    stat_t *s = kind == 0 ? &st_strcmpi : kind == 1 ? &st_strnicmp : &st_strncmp;
    int raw;
    begin(s, edge);
    kit_str(a);
    kit_str(b);
    if (kind)
        kit_i32(n);
    if (kind == 0)
        raw = _strcmpi(a, b);
    else if (kind == 1)
        raw = _strnicmp(a, b, (size_t)n);
    else
        raw = call_strncmp(a, b, (size_t)n);
    raw = out_i(raw);
    finish();
    return raw;
}

static void test_cmp_all(const char *a, const char *b, int edge, int counts)
{
    int n, m = (int)strlen(a), l = (int)strlen(b);
    if (l > m)
        m = l;
    test_cmp_case(0, a, b, 0, edge);
    for (n = counts ? 0 : 1; n <= (counts ? m + 2 : 2); n++) {
        test_cmp_case(1, a, b, n, edge);
        test_cmp_case(2, a, b, n, edge);
    }
}

static int packed_result(int v)
{
    if (v < -128 || v > 127) {
        printf("  compare_bytes: result %d does not fit a byte\n", v);
        pack_bad = 1;
    }
    return v;
}

static void test_cmp_bytes(void)
{
    static signed char r[5][256];
    char a[2], b[2];
    int i, j, n;
    kit_layout("compare_bytes", "in a i32\nout strcmpi bytes256\nout strnicmp1 bytes256\nout strnicmp2 bytes256\n"
               "out strncmp1 bytes256\nout strncmp2 bytes256\n");
    a[1] = b[1] = 0;
    for (i = 0; i < 256; i++) {
        a[0] = (char)i;
        kit_quiet = 1;
        for (j = 0; j < 256; j++) {
            b[0] = (char)j;
            r[0][j] = (signed char)packed_result(test_cmp_case(0, a, b, 0, 1));
            for (n = 1; n <= 2; n++) {
                r[n][j] = (signed char)packed_result(test_cmp_case(1, a, b, n, 1));
                r[2 + n][j] = (signed char)packed_result(test_cmp_case(2, a, b, n, 1));
            }
        }
        kit_quiet = 0;
        kit_begin("compare_bytes", 1);
        kit_i32(i);
        kit_bytes(r, sizeof(r));
        kit_end();
    }
}

static void test_cmp(void)
{
    static const char *const pool[] = {
        "", "a", "A", "b", "B", "Z", "z", "[", "\\", "]", "^", "_", "`", "0", "9", " ", ".", "ab", "AB", "aB", "abc",
        "ABC", "abd", "abcd", "ABCD", "a_", "A_", "a`", "A`", "a[", "aZ", "az", "a\240", "A\300", "\340", "\377",
        "\200", "\337", "[a", "_B", "`b", "^", "Z[", "z{", "abc.", "abc ", "a b", "\351", "\311", "\351a", "\311A",
        "_a", "_A", "\\\\", "[\\]^_`", "{|}~"};
    char sa[16], sb[16];
    int n = (int)(sizeof(pool) / sizeof(pool[0]));
    int i, j, k, len;
    kit_layout("strcmpi", "in a str\nin b str\nout v i32\n");
    kit_layout("strnicmp", "in a str\nin b str\nin n i32\nout v i32\n");
    kit_layout("strncmp", "in a str\nin b str\nin n i32\nout v i32\n");
    for (i = 0; i < n; i++)
        for (j = 0; j < n; j++)
            test_cmp_all(pool[i], pool[j], 1, 1);
    test_cmp_bytes();
    for (k = 0; k < 20000; k++) {
        int la = rndn(13);
        int mode = rndn(5);
        for (i = 0; i < la; i++)
            sa[i] = (char)rchar();
        sa[la] = 0;
        len = la;
        if (mode == 0) {
            len = rndn(13);
            for (i = 0; i < len; i++)
                sb[i] = (char)rchar();
        } else if (mode == 1) {
            for (i = 0; i < la; i++) {
                int c = (unsigned char)sa[i];
                if (rndn(2) && ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')))
                    c ^= 0x20;
                sb[i] = (char)c;
            }
        } else if (mode == 2) {
            len = rndn(la + 1);
            memcpy(sb, sa, len);
        } else if (mode == 3) {
            memcpy(sb, sa, la);
            len = la + 1 + rndn(12 - la > 0 ? 12 - la : 1);
            if (len > 15)
                len = 15;
            for (i = la; i < len; i++)
                sb[i] = (char)rchar();
        } else {
            memcpy(sb, sa, la);
            if (la > 0) {
                int p = rndn(la);
                int c = (unsigned char)sb[p];
                if (rndn(2) && ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')))
                    sb[p] = (char)(c ^ 0x20);
                else
                    sb[p] = (char)rchar();
            }
        }
        sb[len] = 0;
        {
            int m = la > len ? la : len;
            int cnt = rndn(m + 3);
            test_cmp_case(0, sa, sb, 0, 0);
            test_cmp_case(1, sa, sb, cnt, 0);
            test_cmp_case(2, sa, sb, cnt, 0);
        }
    }
}

static void test_charfn_case(stat_t *s, int c, int v, int table)
{
    begin(s, 1);
    kit_i32(c);
#ifdef BX_REPLAY
    if (table && c < -1) {
        int w = (int)vec_get();
        kit_i32(w);
        s->skipped++;
    } else {
        out_i(v);
    }
#else
    (void)table;
    out_i(v);
#endif
    finish();
}

static void test_charfns(void)
{
    int (*volatile fn[5])(int);
    int c;
    fn[0] = tolower;
    fn[1] = toupper;
    fn[2] = isspace;
    fn[3] = isdigit;
    fn[4] = isalnum;
    kit_layout("tolower", "in c i32\nout v i32\n");
    kit_layout("toupper", "in c i32\nout v i32\n");
    kit_layout("isspace", "in c i32\nout v i32\n");
    kit_layout("isdigit", "in c i32\nout v i32\n");
    kit_layout("isalnum", "in c i32\nout v i32\n");
    for (c = -128; c <= 255; c++) {
        test_charfn_case(&st_tolower, c, fn[0](c), 0);
        test_charfn_case(&st_toupper, c, fn[1](c), 0);
        test_charfn_case(&st_isspace, c, fn[2](c), 1);
        test_charfn_case(&st_isdigit, c, fn[3](c), 1);
        test_charfn_case(&st_isalnum, c, fn[4](c), 1);
    }
}

static void test_strlwr_case(const char *s, int edge)
{
    char buf[48];
    strcpy(buf, s);
    begin(&st_strlwr, edge);
    kit_str(s);
    _strlwr(buf);
    out_str(buf);
    finish();
}

static void test_strlwr(void)
{
    char buf[48];
    int i, j, n;
    kit_layout("strlwr", "in s str\nout s str\n");
    buf[1] = 0;
    for (i = 1; i < 256; i++) {
        buf[0] = (char)i;
        test_strlwr_case(buf, 1);
    }
    test_strlwr_case("", 1);
    test_strlwr_case("ABCXYZ[\\]^_`abcxyz@{", 1);
    test_strlwr_case("Hello World 123", 1);
    test_strlwr_case("\300\311\336\337\340\377\200\240", 1);
    for (i = 0; i < 6000; i++) {
        n = rndn(21);
        for (j = 0; j < n; j++)
            buf[j] = (char)(rndn(4) ? rchar() : rbyte());
        buf[n] = 0;
        test_strlwr_case(buf, 0);
    }
}

static void test_strncpy_case(const char *src, int count, int edge)
{
    char dst[24];
    memset(dst, 0xAA, sizeof(dst));
    begin(&st_strncpy, edge);
    kit_str(src);
    kit_i32(count);
    call_strncpy(dst, src, (size_t)count);
    out_bytes(dst, 24);
    finish();
}

static void test_strncpy(void)
{
    char src[24];
    int len, count, i;
    kit_layout("strncpy", "in src str\nin n i32\nout dst bytes24\n");
    for (len = 0; len <= 20; len++) {
        for (i = 0; i < len; i++)
            src[i] = (char)('A' + i);
        src[len] = 0;
        for (count = 0; count <= 24; count++)
            test_strncpy_case(src, count, 1);
    }
    for (i = 0; i < 4000; i++) {
        int j;
        len = rndn(21);
        for (j = 0; j < len; j++)
            src[j] = (char)rbyte();
        src[len] = 0;
        test_strncpy_case(src, rndn(25), 0);
    }
}

static const char *const srch_alpha = "ab\351";
static char srch_hay[121][8];
static int srch_hays;
static char srch_set[40][8];
static int srch_sets;

static void srch_build(void)
{
    int len, i, j, n, total;
    srch_hays = srch_sets = 0;
    for (len = 0; len <= 4; len++) {
        total = 1;
        for (i = 0; i < len; i++)
            total *= 3;
        for (n = 0; n < total; n++) {
            int v = n;
            for (j = 0; j < len; j++) {
                srch_hay[srch_hays][j] = srch_alpha[v % 3];
                v /= 3;
            }
            srch_hay[srch_hays][len] = 0;
            if (len <= 3) {
                memcpy(srch_set[srch_sets], srch_hay[srch_hays], 8);
                srch_sets++;
            }
            srch_hays++;
        }
    }
}

static int offset_of(const char *base, const char *p)
{
    return p == NULL ? -1 : (int)(p - base);
}

static void test_search_case(int kind, const char *hay, const char *needle, int c, int edge)
{
    static stat_t *const stats[] = {&st_strstr, &st_strchr, &st_strrchr, &st_strcspn};
    int r;
    begin(stats[kind], edge);
    kit_str(hay);
    if (kind == 0 || kind == 3)
        kit_str(needle);
    else
        kit_i32(c);
    if (kind == 0)
        r = offset_of(hay, call_strstr(hay, needle));
    else if (kind == 1)
        r = offset_of(hay, call_strchr(hay, c));
    else if (kind == 2)
        r = offset_of(hay, call_strrchr(hay, c));
    else
        r = (int)call_strcspn(hay, needle);
    out_i(r);
    finish();
}

static void test_search(void)
{
    static const int chars[] = {0, 'a', 'b', 'c', 0351, -23, 0x161, 0x1e9, -1, 255, 0x100, 'A'};
    static const char rnd_alpha[] = {'a', 'b', 'c', 'A', (char)0351, (char)0377};
    char hay[24], needle[8];
    int i, j, k;
    kit_layout("strstr", "in hay str\nin needle str\nout off i32\n");
    kit_layout("strchr", "in hay str\nin c i32\nout off i32\n");
    kit_layout("strrchr", "in hay str\nin c i32\nout off i32\n");
    kit_layout("strcspn", "in hay str\nin set str\nout n i32\n");
    srch_build();
    for (i = 0; i < srch_hays; i++) {
        for (j = 0; j < srch_sets; j++) {
            test_search_case(0, srch_hay[i], srch_set[j], 0, 1);
            test_search_case(3, srch_hay[i], srch_set[j], 0, 1);
        }
        for (j = 0; j < (int)(sizeof(chars) / sizeof(chars[0])); j++) {
            test_search_case(1, srch_hay[i], "", chars[j], 1);
            test_search_case(2, srch_hay[i], "", chars[j], 1);
        }
    }
    for (k = 0; k < 10000; k++) {
        int lh = rndn(13), ln = rndn(5), c;
        for (i = 0; i < lh; i++)
            hay[i] = rnd_alpha[rndn(6)];
        hay[lh] = 0;
        if (lh > 0 && rndn(2)) {
            int start = rndn(lh);
            ln = rndn(lh - start + 1);
            if (ln > 5)
                ln = 5;
            memcpy(needle, hay + start, ln);
        } else {
            for (i = 0; i < ln; i++)
                needle[i] = rnd_alpha[rndn(6)];
        }
        needle[ln] = 0;
        c = rndn(5) == 0 ? chars[rndn((int)(sizeof(chars) / sizeof(chars[0])))] : (unsigned char)rnd_alpha[rndn(6)];
        test_search_case(0, hay, needle, 0, 0);
        test_search_case(1, hay, "", c, 0);
        test_search_case(2, hay, "", c, 0);
        test_search_case(3, hay, needle, 0, 0);
    }
}

static void test_strtok_case(const char *s, const char *d1, const char *d2, int edge)
{
    char buf[48];
    int toks[MAXTOK * 2];
    int n = 0, i;
    char *t;
    memset(buf, 0xAA, sizeof(buf));
    strcpy(buf, s);
    memset(toks, 0, sizeof(toks));
    begin(&st_strtok, edge);
    kit_str(s);
    kit_str(d1);
    kit_str(d2);
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
    out_i(n);
    out_bytes(toks, (int)sizeof(toks));
    out_bytes(buf, 48);
    finish();
}

static void test_strtok(void)
{
    static const char *const pool[] = {
        "", " ", "   ", "a", " a", "a ", " a ", "a b", "a  b", "  a  b  ", ",a,,b,", "a,b;c d", ",,,", "abc",
        ";;a;;b;;c;;", "\351 \351", "a b c d e f g h i j k l m n o p q r s t", "x\ty\nz", "ab", "ba"};
    static const char *const delims[] = {" ", " ,", ",;", "ab", "", "\t\n", ",", "\351", " ,;\t"};
    char buf[48];
    static const char tokchars[] = {'a', 'b', 'c', ' ', ',', ';', '\t', (char)0351, 'x'};
    int i, j, k, n;
    int np = (int)(sizeof(pool) / sizeof(pool[0]));
    int nd = (int)(sizeof(delims) / sizeof(delims[0]));
    kit_layout("strtok",
               "in s str\nin delims str\nin delims2 str\nout count i32\nout toks bytes192\nout buf bytes48\n");
    for (i = 0; i < np; i++)
        for (j = 0; j < nd; j++) {
            test_strtok_case(pool[i], delims[j], delims[j], 1);
            test_strtok_case(pool[i], delims[j], delims[(j + 3) % nd], 1);
            test_strtok_case(pool[i], delims[j], " ", 1);
        }
    for (i = 0; i < 6000; i++) {
        n = rndn(40);
        for (k = 0; k < n; k++)
            buf[k] = tokchars[rndn(i % 3 == 0 ? 9 : 6)];
        buf[n] = 0;
        j = rndn(nd);
        test_strtok_case(buf, delims[j], rndn(5) ? delims[j] : delims[rndn(nd)], 0);
    }
}

static void test_memmove_case(int d, int s, int n, int edge)
{
    unsigned char buf[64];
    unsigned char *r;
    int i;
    for (i = 0; i < 64; i++)
        buf[i] = (unsigned char)(i * 37 + 11);
    begin(&st_memmove, edge);
    kit_i32(d);
    kit_i32(s);
    kit_i32(n);
    r = (unsigned char *)call_memmove(buf + d, buf + s, (size_t)n);
    out_i((int)(r - buf));
    out_bytes(buf, 64);
    finish();
}

static void test_memmove(void)
{
    static const int offs[] = {0, 1, 2, 3, 7, 8, 15, 16, 31, 32, 33, 48, 60, 63};
    static const int lens[] = {0, 1, 2, 3, 4, 5, 8, 15, 16, 17, 31, 32, 64};
    int i, j, k;
    int no = (int)(sizeof(offs) / sizeof(offs[0]));
    int nl = (int)(sizeof(lens) / sizeof(lens[0]));
    kit_layout("memmove", "in dst i32\nin src i32\nin n i32\nout ret i32\nout buf bytes64\n");
    for (i = 0; i < no; i++)
        for (j = 0; j < no; j++)
            for (k = 0; k < nl; k++)
                if (offs[i] + lens[k] <= 64 && offs[j] + lens[k] <= 64)
                    test_memmove_case(offs[i], offs[j], lens[k], 1);
    for (i = 0; i < 12000; i++) {
        int n = rndn(65);
        int d = rndn(65 - n);
        int s = rndn(2) ? rndn(65 - n) : d + rndn(9) - 4;
        if (s < 0 || s + n > 64)
            s = rndn(65 - n);
        test_memmove_case(d, s, n, 0);
    }
}

typedef struct {
    int n;
    char kind[MAXCONV + 1];
    char type[MAXCONV + 1];
    int nspec;
    char spec[MAXCONV + 4][16];
} plan_t;

typedef struct {
    int iv[MAXCONV];
    double dv[MAXCONV];
    char sv[MAXCONV][STR_BYTES];
} args_t;

static char dspec[160][16];
static int ndspec;

static void add_dspec(const char *s)
{
    int i;
    for (i = 0; i < ndspec; i++)
        if (strcmp(dspec[i], s) == 0)
            return;
    if (ndspec < 160)
        strcpy(dspec[ndspec++], s);
}

static void parse_plan(const char *f, plan_t *pl)
{
    memset(pl, 0, sizeof(*pl));
    while (*f) {
        if (*f == '%') {
            const char *start = f++;
            char t;
            int len;
            if (*f == '%') {
                f++;
                len = (int)(f - start);
                memcpy(pl->spec[pl->nspec], start, len);
                pl->nspec++;
                continue;
            }
            while (*f == '-' || *f == '+' || *f == ' ' || *f == '#' || *f == '0')
                f++;
            while (*f >= '0' && *f <= '9')
                f++;
            if (*f == '.') {
                f++;
                while (*f >= '0' && *f <= '9')
                    f++;
            }
            while (*f == 'l' || *f == 'h' || *f == 'L')
                f++;
            t = *f++;
            len = (int)(f - start);
            memcpy(pl->spec[pl->nspec], start, len);
            pl->nspec++;
            pl->type[pl->n] = t;
            pl->kind[pl->n] = (t == 's') ? 's' : (t == 'f' || t == 'e' || t == 'g' || t == 'E' || t == 'G') ? 'f' : 'i';
            pl->n++;
        } else {
            f++;
        }
    }
}

static int ipool[640];
static int nipool;
static double dpool[1200];
static int ndpool;
static const char *const spool[] = {
    "", "a", "hello", "Hello World", "0123456789", "%d %s %%", "\351\377\200abc\240",
    "a very long string of fifty characters or so.........", "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
    "  leading and trailing  ", "\t", "\\path\\to\\file.ext", "%", "-12", "ABCDEFGHIJKLMNOPQRSTUVWXYZ"};
#define NSPOOL ((int)(sizeof(spool) / sizeof(spool[0])))

static void add_i(int v)
{
    if (nipool < (int)(sizeof(ipool) / sizeof(ipool[0])))
        ipool[nipool++] = v;
}

static void add_d(double v)
{
    if (ndpool < (int)(sizeof(dpool) / sizeof(dpool[0])))
        dpool[ndpool++] = v;
}

static double make_double(bx_u32 hi, bx_u32 lo)
{
    union {
        double d;
        bx_u32 w[2];
    } b;
    b.w[0] = lo;
    b.w[1] = hi;
    return b.d;
}

static void split_double(double d, bx_u32 *hi, bx_u32 *lo)
{
    union {
        double d;
        bx_u32 w[2];
    } b;
    b.d = d;
    *lo = b.w[0];
    *hi = b.w[1];
}

static void bump(bx_u32 *hi, bx_u32 *lo, int up)
{
    if (((*hi & 0x80000000u) != 0) == (up != 0)) {
        if ((*lo)-- == 0)
            (*hi)--;
    } else {
        if (++(*lo) == 0)
            (*hi)++;
    }
}

static double next_up(double d)
{
    bx_u32 hi, lo;
    split_double(d, &hi, &lo);
    bump(&hi, &lo, 1);
    return make_double(hi, lo);
}

static double next_down(double d)
{
    bx_u32 hi, lo;
    split_double(d, &hi, &lo);
    bump(&hi, &lo, 0);
    return make_double(hi, lo);
}

static double pow10d[23];

static double ratio(int num, int den)
{
    volatile double a = (double)num;
    volatile double b = (double)den;
    double r = a / b;
    return r;
}

static void init_pools(void)
{
    static const int fixed[] = {0, 1, -1, 2, -2, 3, 7, 9, 10, 11, -9, -10, -11, 99, 100, 101, 255, 256, 257, -255,
                                -256, 0x7fff, 0x8000, 0xffff, 0x10000, -0x8000, -0x8001, -0x10000, INT_MAX,
                                INT_MAX - 1, INT_MIN, INT_MIN + 1, 0x12345678, -0x12345678, 0xdeadbeef, 0x80000001};
    int i, k, p, n;
    nipool = 0;
    for (i = 0; i < (int)(sizeof(fixed) / sizeof(fixed[0])); i++)
        add_i(fixed[i]);
    for (p = 1, k = 0; k < 8; k++, p *= 16) {
        add_i(p);
        add_i(p - 1);
        add_i(p + 1);
        add_i(-p);
    }
    for (p = 1, k = 0; k < 10; k++, p *= 10) {
        add_i(p);
        add_i(p - 1);
        add_i(p + 1);
        add_i(-p);
        add_i(-(p - 1));
        add_i(5 * p);
    }
    for (i = 3; i < 200; i += 7)
        add_i(i * 1013 - 100000);
    pow10d[0] = 1.0;
    for (i = 1; i < 23; i++)
        pow10d[i] = pow10d[i - 1] * 10.0;
    ndpool = 0;
    add_d(0.0);
    add_d(make_double(0x80000000u, 0));
    add_d(1.0);
    add_d(-1.0);
    add_d(0.5);
    add_d(-0.5);
    add_d(1.5);
    add_d(2.5);
    add_d(-2.5);
    add_d(3.5);
    add_d(0.125);
    add_d(0.375);
    add_d(pow10d[15]);
    add_d(pow10d[16]);
    add_d(pow10d[21]);
    add_d(pow10d[22]);
    add_d(-pow10d[15]);
    add_d(pow10d[9]);
    add_d(4294967296.0);
    add_d(2147483648.0);
    add_d(9007199254740992.0);
    add_d(9007199254740994.0);
    add_d(123456789.0);
    add_d(123456789.5);
    add_d(make_double(0x7e300000u, 0));
    add_d(make_double(0x01b00000u, 0));
    add_d(make_double(0x7fefffffu, 0xffffffffu));
    add_d(make_double(0xffefffffu, 0xffffffffu));
    add_d(make_double(0, 1));
    add_d(make_double(0x000fffffu, 0xffffffffu));
    add_d(make_double(0x00100000u, 0));
    add_d(make_double(0x7ff00000u, 0));
    add_d(make_double(0xfff00000u, 0));
    add_d(make_double(0x7ff80000u, 0));
    add_d(make_double(0xfff80000u, 0));
    for (i = 1; i <= 3; i++)
        for (n = 1; n <= 9; n++) {
            add_d(ratio(i, n));
            add_d(-ratio(i, n));
        }
    for (p = 0; p < 8; p++)
        for (n = 0; n < 25; n++) {
            double v = ratio(2 * n + 1, 2 * (int)pow10d[p]);
            add_d(v);
            if (n < 8) {
                add_d(next_up(v));
                add_d(next_down(v));
                add_d(-v);
            }
        }
    add_d(ratio(9995, 100));
    add_d(ratio(995, 1000));
    add_d(ratio(9995, 1000));
    add_d(ratio(9999999, 10000000));
    add_d(ratio(99999, 100000));
    add_d(ratio(999999, 1000000));
    add_d(ratio(9999995, 10000000));
    add_d(ratio(1, 10));
    add_d(ratio(2, 10));
    add_d(ratio(3, 10));
    add_d(ratio(1, 100));
    add_d(ratio(1, 1000));
    add_d(ratio(1, 1000000));
    add_d(ratio(1, 10000000));
    add_d(ratio(1, 100000000));
}

static double rdbl(void)
{
    int mode = rndn(4);
    double d;
    if (mode == 0) {
        bx_u32 hi = (bx_u32)rndn(2) << 31;
        hi |= (bx_u32)(1023 - 80 + rndn(200)) << 20;
        hi |= rnd32() & 0xfffffu;
        return make_double(hi, rnd32());
    }
    if (mode == 3) {
        long whole = (long)(rnd32() >> 1);
        int half = rndn(2);
        int neg = rndn(2);
        d = (double)whole + (half ? 0.5 : 0.0);
        return neg ? -d : d;
    }
    {
        long whole = (long)(rnd32() % 2000000);
        int scale = rndn(8);
        d = (double)whole / pow10d[scale];
    }
    return mode == 1 ? d : -d;
}

static void fill_args(const plan_t *pl, args_t *a, int edge, int k)
{
    int j;
    memset(a, 0, sizeof(*a));
    for (j = 0; j < pl->n; j++) {
        if (pl->kind[j] == 'i') {
            if (pl->type[j] == 'c')
                a->iv[j] = edge ? (k == 0 ? 65 : ((k * 29 + j * 7) % 384) - 128) : rndn(384) - 128;
            else if (edge)
                a->iv[j] = ipool[k == 0 ? 0 : (k * 37 + j * 11) % nipool];
            else {
                a->iv[j] = (int)rnd32();
                if (rndn(2))
                    a->iv[j] >>= rndn(32);
            }
        } else if (pl->kind[j] == 's') {
            if (edge) {
                strcpy(a->sv[j], spool[k == 0 ? 0 : (k * 3 + j * 5) % NSPOOL]);
            } else {
                int n = rndn(40), i;
                for (i = 0; i < n; i++)
                    a->sv[j][i] = (char)(rndn(3) ? rchar() : rbyte());
            }
        } else {
            a->dv[j] = edge ? dpool[k == 0 ? 0 : (k * 41 + j * 13) % ndpool] : rdbl();
        }
    }
}

static int call_sprintf(char *buf, const char *fmt, const plan_t *pl, const args_t *a)
{
    static char fcopy[256];
    unsigned long w[20];
    int j, p = 0;
    memset(w, 0, sizeof(w));
    for (j = 0; j < pl->n; j++) {
        if (pl->kind[j] == 'i') {
            w[p++] = (unsigned long)a->iv[j];
        } else if (pl->kind[j] == 's') {
            w[p++] = (unsigned long)a->sv[j];
        } else {
            memcpy(&w[p], &a->dv[j], 8);
            p += 2;
        }
    }
    strcpy(fcopy, fmt);
    return sprintf(buf, fcopy, w[0], w[1], w[2], w[3], w[4], w[5], w[6], w[7], w[8], w[9], w[10], w[11], w[12], w[13],
                   w[14], w[15]);
}

static void kit_args(const plan_t *pl, const args_t *a)
{
#ifdef BX_KIT
    static unsigned char buf[MAXCONV * (4 + STR_BYTES)];
    int j, n, p = 0;
    for (j = 0; j < pl->n; j++) {
        if (pl->kind[j] == 'i') {
            bx_u32 v = (bx_u32)a->iv[j];
            buf[p++] = (unsigned char)v;
            buf[p++] = (unsigned char)(v >> 8);
            buf[p++] = (unsigned char)(v >> 16);
            buf[p++] = (unsigned char)(v >> 24);
        } else if (pl->kind[j] == 's') {
            n = (int)strlen(a->sv[j]);
            buf[p++] = (unsigned char)n;
            buf[p++] = (unsigned char)(n >> 8);
            buf[p++] = 0;
            buf[p++] = 0;
            memcpy(buf + p, a->sv[j], n);
            p += n;
        } else {
            memcpy(buf + p, &a->dv[j], 8);
            p += 8;
        }
    }
    kit_u32((bx_u32)p);
    kit_bytes(buf, p);
#else
    (void)pl;
    (void)a;
#endif
}

static void test_sprintf_case(const char *fmt, const plan_t *pl, const args_t *a, int edge)
{
    static char buf[8192];
    int ret;
    memset(buf, 0xAA, sizeof(buf));
    begin(&st_sprintf, edge);
    cur_note = fmt;
    kit_str(fmt);
    kit_str(pl->kind);
    kit_args(pl, a);
    ret = call_sprintf(buf, fmt, pl, a);
    out_str_n(buf, ret < 0 ? 0 : ret);
    out_i(ret);
    finish();
}

static void test_sprintf(void)
{
    int nf = (int)(sizeof(sprintf_formats) / sizeof(sprintf_formats[0]));
    int i, j, k, count;
    plan_t pl;
    args_t a;
    kit_layout("sprintf", "in fmt str\nin kinds str\nin args str\nout s str\nout ret i32\n");
    init_pools();
    for (i = 0; i < nf; i++) {
        parse_plan(sprintf_formats[i], &pl);
        for (j = 0; j < pl.nspec; j++)
            add_dspec(pl.spec[j]);
    }
    for (i = 0; i < ndspec; i++) {
        parse_plan(dspec[i], &pl);
        if (pl.n == 0) {
            memset(&a, 0, sizeof(a));
            test_sprintf_case(dspec[i], &pl, &a, 1);
            continue;
        }
        count = pl.kind[0] == 'f' ? ndpool : pl.kind[0] == 's' ? NSPOOL : pl.type[0] == 'c' ? 384 : nipool;
        for (k = 0; k < count; k++) {
            memset(&a, 0, sizeof(a));
            if (pl.kind[0] == 'f')
                a.dv[0] = dpool[k];
            else if (pl.kind[0] == 's')
                strcpy(a.sv[0], spool[k]);
            else
                a.iv[0] = pl.type[0] == 'c' ? k - 128 : ipool[k];
            test_sprintf_case(dspec[i], &pl, &a, 1);
        }
        for (k = 0; k < 100; k++) {
            fill_args(&pl, &a, 0, k);
            test_sprintf_case(dspec[i], &pl, &a, 0);
        }
    }
    for (i = 0; i < nf; i++) {
        parse_plan(sprintf_formats[i], &pl);
        for (k = 0; k < 12; k++) {
            fill_args(&pl, &a, 1, k);
            test_sprintf_case(sprintf_formats[i], &pl, &a, 1);
        }
        for (k = 0; k < 40; k++) {
            fill_args(&pl, &a, 0, k);
            test_sprintf_case(sprintf_formats[i], &pl, &a, 0);
        }
    }
}

static void test_sscanf_case(const char *fmt, const char *text, int edge)
{
    static char fcopy[128];
    char tcopy[128];
    int ints[4];
    char sb[64];
    void *ptr[6];
    int ni = 0, ns = 0, np = 0, ret;
    const char *f;
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
            ns++;
        } else if (*f == 's') {
            ptr[np++] = sb;
            ns++;
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
    begin(&st_sscanf, edge);
    cur_note = fmt;
    kit_str(fmt);
    kit_str(text);
    ret = sscanf(tcopy, fcopy, ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5]);
    out_i(ret);
    out_bytes(ints, (int)sizeof(ints));
    out_bytes(sb, 64);
    finish();
}

static void test_sscanf(void)
{
    static const char *const pool[] = {
        "", " ", "\t\n\v\f\r", "0", "1", "12", "-1", "+1", "-", "+", " 12", "\t\n 12", "12 34", "12 34 56", "12abc",
        "abc", "abc 12", "name.ext 5", "a.b", "A_b.c9 -7 +8", "Ab,12", "Ab,-12", "ab,12,34,56", "ab, 12", "Ab 12",
        ",12", "1,2", "2147483647", "2147483648", "-2147483648", "-2147483649", "4294967295", "4294967296",
        "99999999999999999999", "0x1f", "0X1F", "1f", "FF", "ff", "-ff", "0x", "0xg", "010", "08", "0012",
        "\240\240", "\351,5", "a\351,7", "ab\377 5", "abc def", "abc\tdef", "   abc   ", "abc  def  ghi", "12\n34",
        "a-b", "zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz", "A,1,2,3,4", "x,y", "q,0x10,010,-5",
        "word 7 trailing", "9 8 7"};
    static const char *const letters = "abcXYZ019_.";
    int nf = (int)(sizeof(sscanf_formats) / sizeof(sscanf_formats[0]));
    int np = (int)(sizeof(pool) / sizeof(pool[0]));
    char text[128];
    int i, j, k;
    kit_layout("sscanf", "in fmt str\nin text str\nout ret i32\nout ints bytes16\nout s bytes64\n");
    for (i = 0; i < nf; i++)
        for (j = 0; j < np; j++)
            test_sscanf_case(sscanf_formats[i], pool[j], 1);
    for (i = 0; i < nf; i++)
        for (k = 0; k < 600; k++) {
            int pieces = 1 + rndn(5);
            text[0] = 0;
            for (j = 0; j < pieces; j++) {
                int len = (int)strlen(text);
                int kind = rndn(7), n, m;
                if (len > 40)
                    break;
                n = kind == 0 ? rndn(4) : kind == 1 ? 1 : kind == 2 ? 1 + rndn(12) : kind == 3 ? 1 + rndn(6) : 1;
                for (m = 0; m < n; m++) {
                    char c;
                    if (kind == 0)
                        c = " \t\n\v\f\r"[rndn(6)];
                    else if (kind == 1)
                        c = rndn(2) ? '+' : '-';
                    else if (kind == 2)
                        c = (char)('0' + rndn(10));
                    else if (kind == 3)
                        c = letters[rndn(11)];
                    else if (kind == 4)
                        c = ",.;-x"[rndn(5)];
                    else if (kind == 5)
                        c = (char)(0x80 + rndn(128));
                    else
                        c = (char)(rndn(2) ? 'a' + rndn(26) : 'A' + rndn(26));
                    text[len + m] = c;
                }
                text[len + n] = 0;
            }
            test_sscanf_case(sscanf_formats[i], text, 0);
        }
}

static int ftol_call(double d)
{
    return (int)d;
}

static void test_ftol_case(double d, int edge)
{
    int v;
    bx_u32 hi, lo;
    begin(&st_ftol, edge);
    kit_f64(d);
    split_double(d, &hi, &lo);
    v = ftol_call(d);
#ifdef BX_REPLAY
    if ((bx_u32)vec_get() != hi)
        cur_bad = 1;
    if ((bx_u32)vec_get() != lo)
        cur_bad = 1;
    {
        int w = (int)vec_get();
        kit_i32(w);
        if (bx_ftol(d) != w)
            cur_bad = 1;
        (void)v;
    }
#else
    vec_put(hi);
    vec_put(lo);
    out_i(v);
    if (bx_ftol(d) != v)
        cur_bad = 1;
#endif
    finish();
}

static int signaling_nan(bx_u32 hi, bx_u32 lo)
{
    return (hi & 0x7ff00000u) == 0x7ff00000u && ((hi & 0xfffffu) | lo) != 0 && !(hi & 0x80000u);
}

static void test_ftol(void)
{
    static const bx_u32 bases[][2] = {
        {0x00000000u, 0}, {0x3fe00000u, 0}, {0x3ff00000u, 0}, {0x3ff80000u, 0}, {0x40040000u, 0},
        {0x41dfffffu, 0xffc00000u}, {0x41e00000u, 0}, {0x41f00000u, 0}, {0x43300000u, 0}, {0x43400000u, 0},
        {0x43e00000u, 0}, {0x7e300000u, 0}, {0x7fefffffu, 0xffffffffu}, {0x7ff00000u, 0}, {0x7ff80000u, 0},
        {0x00000000u, 1}, {0x000fffffu, 0xffffffffu}, {0x00100000u, 0}, {0x3fefffffu, 0xffffffffu},
        {0x41dfffffu, 0xffe00000u}, {0x41e00000u, 0x00200000u}, {0x40590000u, 0}, {0x40c38800u, 0}};
    int i, s;
    kit_layout("ftol", "in d f64\nout v i32\n");
    for (i = 0; i < (int)(sizeof(bases) / sizeof(bases[0])); i++)
        for (s = 0; s < 2; s++) {
            bx_u32 hi = bases[i][0] | (s ? 0x80000000u : 0), lo = bases[i][1];
            bx_u32 uh = hi, ul = lo, dh = hi, dl = lo;
            bump(&uh, &ul, 1);
            bump(&dh, &dl, 0);
            test_ftol_case(make_double(hi, lo), 1);
            if (!signaling_nan(uh, ul))
                test_ftol_case(make_double(uh, ul), 1);
            if (!signaling_nan(dh, dl))
                test_ftol_case(make_double(dh, dl), 1);
        }
    for (i = 0; i < 8000; i++) {
        int e = i % 4 == 0 ? 1023 + 28 + rndn(8) : 1023 - 40 + rndn(130);
        bx_u32 hi = (bx_u32)rndn(2) << 31;
        hi |= (bx_u32)e << 20;
        hi |= rnd32() & 0xfffffu;
        test_ftol_case(make_double(hi, rnd32()), 0);
    }
}

int main(int argc, char **argv)
{
    int i, bad = 0;
    if (argc > 1 && !vec_open(argv[1]))
        return 2;
    kit_open(argc > 2 ? argv[2] : NULL);
    test_atoi();
    test_itoa();
    test_cmp();
    test_charfns();
    test_strlwr();
    test_strncpy();
    test_search();
    test_strtok();
    test_memmove();
    test_sprintf();
    test_sscanf();
    test_ftol();
    bad |= kit_close();
    bad |= pack_bad;
    if (argc > 1)
        bad |= vec_close();
    for (i = 0; i < (int)(sizeof(all_stats) / sizeof(all_stats[0])); i++) {
        printf("%s: %lu cases, %lu mismatches\n", all_stats[i]->name, all_stats[i]->cases, all_stats[i]->mismatches);
        if (all_stats[i]->mismatches)
            bad = 1;
    }
    for (i = 0; i < (int)(sizeof(all_stats) / sizeof(all_stats[0])); i++)
        printf("info: %s %lu edge, %lu random%s\n", all_stats[i]->name, all_stats[i]->edge,
               all_stats[i]->cases - all_stats[i]->edge, all_stats[i]->skipped ? ", not compared" : "");
    return bad;
}
