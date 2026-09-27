/* test/test_palette.c -- ANSI palette and pen layout for deep screens. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "palette.h"

/* The old palette is 12-bit RGB4 in ibmcon's unit-1 order (pen 1 white, pen 7
 * red, 9/15 swapped likewise); the new one is 24-bit in ANSI order
 * (1 red, 7 white). A prefs file without the new block converts once. */
static void test_rgb4_in_ibmcon_order_converts_to_ansi_rgb32(void) {
    struct PrefsStruct p;

    memset(&p, 0, sizeof(p));
    p.color[1] = 0xDDD;   /* ibmcon 1 = white */
    p.color[7] = 0xD00;   /* ibmcon 7 = red */
    p.color[9] = 0xFFF;   /* ibmcon 9 = bright white */
    p.color[15] = 0xF00;  /* ibmcon 15 = bright red */
    p.color[2] = 0x0A0;
    Palette_EnsureAnsi(&p);
    assert(p.ansiMagic == PREFS_ANSI_MAGIC);
    assert(p.ansiRGB[1] == 0xDD0000);    /* ANSI 1 red   */
    assert(p.ansiRGB[7] == 0xDDDDDD);    /* ANSI 7 white */
    assert(p.ansiRGB[9] == 0xFF0000);
    assert(p.ansiRGB[15] == 0xFFFFFF);
    assert(p.ansiRGB[2] == 0x00AA00);

    p.ansiRGB[2] = 0x123456;             /* already converted: left alone */
    Palette_EnsureAnsi(&p);
    assert(p.ansiRGB[2] == 0x123456);
}

/* And back, for screens of 16 colours or fewer (LoadRGB4, ibmcon order). */
static void test_ansi_rgb32_back_to_rgb4(void) {
    struct PrefsStruct p;

    memset(&p, 0, sizeof(p));
    p.ansiMagic = PREFS_ANSI_MAGIC;
    p.ansiRGB[1] = 0xFF0000;
    p.ansiRGB[7] = 0x807F7F;
    Palette_SyncRGB4(&p);
    assert(p.color[7] == 0xF00);          /* ibmcon 7 = ANSI red */
    assert(p.color[1] == 0x877);          /* rounds to nearest: 0x80 -> 8, 0x7F -> 7 */
}

/* Where the 16 ANSI colours live: shared pens 0-15 below 32 colours (no
 * table), otherwise pens of their own that skip the UI pens 0-7 and the AGA
 * pointer pens 16-19. */
static void test_ansi_pen_layout(void) {
    UBYTE pens[16];
    int i;

    assert(!Palette_AnsiPens(4, pens));
    assert(Palette_AnsiPens(5, pens));
    for (i = 0; i < 8; i++)  assert(pens[i] == 8 + i);
    for (i = 8; i < 16; i++) assert(pens[i] == 20 + (i - 8));
    assert(Palette_AnsiPens(8, pens) && pens[15] == 27);
    assert(Palette_AnsiPens(24, pens) && pens[0] == 8);   /* RTG true colour */
}

int main(void) {
    test_rgb4_in_ibmcon_order_converts_to_ansi_rgb32();
    test_ansi_rgb32_back_to_rgb4();
    test_ansi_pen_layout();
    printf("palette: all assertions passed\n");
    return 0;
}
