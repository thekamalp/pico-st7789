#include "font.h"
#include "font/console_font_5x8.h"
#include "font/AIXOID9.h"
#include "font/BLKBOARD.h"
#include "font/BULKY.h"
#include "font/HOLLOW.h"
#include "font/MEDIEVAL.h"
#include "font/SCRAWL2.h"
#include "font/SCRIPT2.h"

const FONT_T console_font_5x8 = { 5, 8, 8, 0, console_font_5x8_glyphs };
const FONT_T AIXOID9_F16 = { 8, 16, 16, 0, AIXOID9_F16_glyphs };
const FONT_T BLKBOARD_F16 = { 8, 16, 16, 0, BLKBOARD_F16_glyphs };
const FONT_T BULKY_F16 = { 8, 16, 16, 0, BULKY_F16_glyphs };
const FONT_T HOLLOW_F16 = { 8, 16, 16, 0, HOLLOW_F16_glyphs };
const FONT_T MEDIEVAL_F16 = { 8, 16, 16, 0, MEDIEVAL_F16_glyphs };
const FONT_T SCRAWL2_F16 = { 8, 16, 16, 0, SCRAWL2_F16_glyphs };
const FONT_T SCRIPT2_F14 = { 8, 14, 14, 0, SCRIPT2_F14_glyphs };

const FONT_T* font[MAX_FONT] = {
    &console_font_5x8,
    &AIXOID9_F16,
    &BLKBOARD_F16,
    &BULKY_F16,
    &HOLLOW_F16,
    &MEDIEVAL_F16,
    &SCRAWL2_F16,
    &SCRIPT2_F14
};

