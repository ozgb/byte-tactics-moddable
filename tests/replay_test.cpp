#include <stdio.h>
#include <string.h>

#include "../src/harness/replay_text.h"

static int g_failures;

static void Check(bool ok, const char* what)
{
    if (!ok) {
        printf("FAIL: %s\n", what);
        g_failures++;
    }
}

static void CheckText(const ReplayText& text, const char* want, const char* what)
{
    if (strcmp(text.Data(), want) != 0) {
        printf("FAIL: %s: got %s, want %s\n", what, text.Data(), want);
        g_failures++;
    }
}

static void TestNumbers()
{
    ReplayText t;
    t.AddInt(-5);
    t.AddChar(' ');
    t.AddUnsigned(4000000000u);
    t.AddChar(' ');
    t.AddHex32(0x20);
    t.AddChar(' ');
    t.AddF32(0x3f800000u);
    t.AddChar(' ');
    t.AddF64(0, 0x3ff00000u);
    CheckText(t, "-5 4000000000 0x00000020 f32:0x3f800000 f64:0x3ff0000000000000", "numbers");
}

static void TestStrings()
{
    ReplayText t;
    t.AddString("gamedata\\meteor");
    t.AddChar(' ');
    t.AddString("a\"b\n\t\x01\xff");
    t.AddChar(' ');
    t.AddString(0);
    t.AddChar(' ');
    t.AddString("");
    CheckText(t, "\"gamedata\\\\meteor\" \"a\\\"b\\n\\t\\x01\\xff\" null \"\"", "strings");
}

static void TestGrowth()
{
    ReplayText t;
    for (int i = 0; i < 5000; i++)
        t.Add("0123456789");
    Check(t.Size() == 50000, "grown size");
    ReplayText u;
    u.Add("x");
    u.AddText(t);
    Check(u.Size() == 50001 && u.Data()[0] == 'x' && u.Data()[50000] == '9', "added text");
    t.Clear();
    Check(t.Size() == 0 && strcmp(t.Data(), "") == 0, "cleared");
}

static void TestKnown()
{
    ReplayKnown k;
    k.Reset();
    Check(k.Changed("a", "1"), "first value is a change");
    Check(!k.Changed("a", "1"), "same value is not");
    Check(k.Changed("a", "2"), "new value is");
    Check(!k.Changed("a", "2"), "new value then stays");
    k.Set("b", "7");
    Check(!k.Changed("b", "7"), "set value is known");
    Check(k.Changed("c", "7"), "other name is a change");
    k.Reset();
    Check(k.Changed("a", "2"), "reset forgets");
}

static void TestKnownMany()
{
    ReplayKnown k;
    char name[32];
    char value[16];
    for (int i = 0; i < 5000; i++) {
        sprintf(name, "Unit#%d.health", i);
        sprintf(value, "%d", i);
        k.Set(name, value);
    }
    bool all = true;
    for (int i = 0; i < 5000; i++) {
        sprintf(name, "Unit#%d.health", i);
        sprintf(value, "%d", i);
        all = all && !k.Changed(name, value) && strcmp(k.Get(name), value) == 0;
    }
    Check(all, "thousands of names are all found again");
    Check(k.Get("Unit#5000.health") == 0, "an unknown name has no value");
}

static void TestOrdinals()
{
    ReplayOrdinals o;
    o.Reset();
    int x, y, z;
    Check(o.Of(0, 0) == 0, "null has no ordinal");
    Check(o.Of(0, &x) == 1 && o.Of(0, &y) == 2 && o.Of(0, &x) == 1, "first use numbers");
    Check(o.Of(1, &x) == 1, "kinds number apart");
    Check(o.Of(1, &z) == 2, "second of the kind");
    o.Reset();
    Check(o.Of(0, &y) == 1, "reset renumbers");
}

int main()
{
    TestNumbers();
    TestStrings();
    TestGrowth();
    TestKnown();
    TestKnownMany();
    TestOrdinals();
    if (g_failures == 0) {
        printf("replay_test: all passed\n");
        return 0;
    }
    printf("replay_test: %d failed\n", g_failures);
    return 1;
}
