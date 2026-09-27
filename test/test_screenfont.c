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

int main(void) {
    test_topaz_on_square_pixels_becomes_topaz_pro();
    printf("screenfont: all assertions passed\n");
    return 0;
}
