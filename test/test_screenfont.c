/* test/test_screenfont.c -- Topaz Pro on square-pixel screens. */
#include <assert.h>
#include <stdio.h>
#include "screenfont.h"

/* Topaz was drawn for the Amiga's tall non-square pixels (PAL hires is 22x44
 * ticks per pixel); on square-pixel modes -- every RTG mode -- it looks
 * squashed. Topaz Pro replaces it there, and only it: a font the user chose
 * is never swapped. */
static void test_topaz_on_square_pixels_becomes_topaz_pro(void) {
    assert(ScreenFont_WantsTopazPro("topaz.font", 44, 44));       /* RTG */
    assert(ScreenFont_WantsTopazPro("Topaz.font", 22, 22));       /* case-insensitive */
    assert(!ScreenFont_WantsTopazPro("topaz.font", 22, 44));      /* PAL hires */
    assert(!ScreenFont_WantsTopazPro("IBM.font", 44, 44));        /* the user's choice */
    assert(!ScreenFont_WantsTopazPro("topaz.font", 0, 0));        /* no display info */
}

/* DCTelnet told the BBS (telnet NAWS) the outer window size: a 640 x 200
 * Workbench window with Topaz Pro 8x16 went out as 80 x 12, while its text
 * area (618 x 184 inside the title bar and borders) holds 77 x 11 -- the grid
 * ibmcon draws. */
static void test_window_size_sent_to_the_bbs_is_the_text_area(void) {
    UWORD cols, rows;

    ScreenFont_Grid(640, 200, 618, 184, TRUE, 8, 16, &cols, &rows);
    assert(cols == 77 && rows == 11);
    ScreenFont_Grid(640, 256, 0, 0, FALSE, 8, 8, &cols, &rows);     /* own screen, no GZZ */
    assert(cols == 80 && rows == 32);
    ScreenFont_Grid(40, 10, 4, 4, TRUE, 8, 16, &cols, &rows);       /* never below 1 x 1 */
    assert(cols == 1 && rows == 1);
    ScreenFont_Grid(1920, 1080, 1900, 1060, TRUE, 8, 16, &cols, &rows);
    assert(cols == 199);                                              /* ibmcon's column cap */
}

/* A Workbench window could be sized wider than 80 columns, and a BBS title
 * drawn for 80 columns wrapped in the wrong places. The window stops at 80
 * columns of text (40 in PETSCII Mode) plus its borders; resized back
 * down, the grid measures 80. */
static void test_window_stops_at_80_columns(void) {
    UWORD max = ScreenFont_MaxWindowWidth(SCREENFONT_ANSI_COLUMNS, 8, 22);
    UWORD cols, rows;

    assert(max == 662);
    ScreenFont_Grid(max, 200, max - 22, 184, TRUE, 8, 16, &cols, &rows);
    assert(cols == 80);
    assert(ScreenFont_MaxWindowWidth(SCREENFONT_PETSCII_COLUMNS, 16, 22) == 662);
}

int main(void) {
    test_window_stops_at_80_columns();
    test_topaz_on_square_pixels_becomes_topaz_pro();
    test_window_size_sent_to_the_bbs_is_the_text_area();
    printf("screenfont: all assertions passed\n");
    return 0;
}
