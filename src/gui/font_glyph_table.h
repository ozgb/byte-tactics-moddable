#ifndef FONT_GLYPH_TABLE_H
#define FONT_GLYPH_TABLE_H

struct GafEntry;

struct FontGlyphTable {
    char unknown_0[3];
    unsigned char first;
    unsigned short offsets[1];
    char unknown_6[6];
    GafEntry* glyphs;
};

#endif
