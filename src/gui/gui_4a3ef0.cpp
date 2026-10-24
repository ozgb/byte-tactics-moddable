#include "gadget.h"

#include "../graphics/gaf_frame.h"

struct Font_004a3ef0 {
    char unknown_0[0x28];
    GafFrame* glyph;
};

struct List_004a3ef0 {
    char unknown_0[0x0c];
    unsigned short* glyphs;
};

struct Holder_004a3ef0 {
    int current;
    Gadget* entries;
    char unknown_08[0x14 - 0x08];
    List_004a3ef0* list;
};

struct Dialog {
    char unknown_0[0x18];
    Holder_004a3ef0* holder;
};

extern Holder_004a3ef0* g_guiContext;

void __stdcall SetFont(int id);
int __stdcall GetGafFrame(unsigned short* seq, int frame);
#include "../graphics/draw.h"
void __stdcall DrawSliderBar(Dialog* gui, int gadgetIndex);

static inline GafFrame* GetGlyph_004a3ef0(unsigned char c)
{
    return (GafFrame*)GetGafFrame(g_guiContext->list->glyphs, c);
}

static inline int Find_004a3ef0(Gadget* entries, unsigned char kind)
{
    for (int i = 1; i < entries->u.count + 1; i++) {
        if (entries[i].type == 2 && entries[i].team == kind)
            return i;
    }
    return 0;
}

static inline int LineSize_004a3ef0(Gadget* e)
{
    int count = e->u.list.rowCount;
    Font_004a3ef0** font = (Font_004a3ef0**)e->u.list.rows;
    int lines = 0;
    if (count > 0)
        lines = (*font)->glyph->height * count;
    return lines;
}

// FUNCTION: 0x4a3ef0
void __stdcall DrawSlider(Dialog* gui, int gadgetIndex)
{
    Gadget* entries = gui->holder->entries;
    Gadget* me = &entries[gadgetIndex];
    unsigned char kind = me->team;
    int found = Find_004a3ef0(entries, kind);
    union { int full; short word; } lines;
    lines.full = 0;
    lines.word = 0;
    if (found != 0) {
        Gadget* e = &entries[found];
        if (e->type == 2) {
            if (e->attribs & 0x10) {
                int i = 1;
                int n = 0;
                for (; i < entries->u.count + 1; i++) {
                    if (entries[i].type == 7) {
                        if (n == e->tab) {
                            SetFont(entries[i].u.list.language);
                            break;
                        }
                        n++;
                    }
                }
                if (i == entries->u.count + 1) {
                    SetFont(g_guiContext->current);
                }
                int size = (g_guiContext->list == 0) ? GetFontHeight()
                    : GetGlyph_004a3ef0(0x49)->height + 2;
                int numerator = e->height - 2;
                int denominator = (e->u.list.scroll > size + 1) ? e->u.list.scroll : size + 1;
                int step = numerator / denominator;
                int last = e->u.list.rowCount;
#ifdef __clang__
                int rows = (int)((double)step / last * (me->height - 3));
#else
                int rows = (int)((float)step / last * (me->height - 3));
#endif
                me->knobSize = rows;
                if (me->knobSize < 10) {
                    me->knobSize = 10;
                }
                if (last <= step) {
                    me->range = 0;
                } else {
                    me->range = me->height - me->knobSize - 3;
                }
            } else if (e->attribs & 0x20) {
                lines.full = LineSize_004a3ef0(e);
                int s = e->height * me->height / lines.full;
                me->knobSize = s;
                if (me->attribs & 1) {
                    me->range = me->width - s;
                } else {
                    me->range = me->height - s;
                }
            } else if (e->attribs & 0x80) {
                if (e->u.list.scroll != 0 && e->u.list.rowCount != 0) {
                    int s = e->height / e->u.list.scroll * me->height / e->u.list.rowCount;
                    me->knobSize = s;
                    if (me->attribs & 1) {
                        me->range = me->width - s;
                    } else {
                        me->range = me->height - s;
                    }
                }
            }
        }
    }
    DrawSliderBar(gui, gadgetIndex);
}
