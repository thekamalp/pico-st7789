// font glyph

#ifndef __FONT_H
#define __FONT_H

#include <stdint.h>

typedef struct FONT_T_ {
    uint8_t width;
    uint8_t height;
    uint8_t stride;
    uint8_t start;
    const uint8_t* glyphs;
} FONT_T;

#define NO_BACKGROUND 0x0020

#define MAX_FONT 8

#define FONT_ID_CONSOLE_5X8 0
#define FONT_ID_AIXOID9_F16 1
#define FONT_ID_BLKBOARD_F16 2
#define FONT_ID_BULKY_F16 3
#define FONT_ID_HOLLOW_F16 4
#define FONT_ID_MEDIEVAL_F16 5
#define FONT_ID_SCRAWL2_F16 6
#define FONT_ID_SCRIPT2_F14 7

extern const FONT_T* font[MAX_FONT];

extern const FONT_T console_font_5x8;
extern const FONT_T AIXOID9_F16;
extern const FONT_T BLKBOARD_F16;
extern const FONT_T BULKY_F16;
extern const FONT_T HOLLOW_F16;
extern const FONT_T MEDIEVAL_F16;
extern const FONT_T SCRAWL2_F16;
extern const FONT_T SCRIPT2_F14;

#endif
