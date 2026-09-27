/* src/palette.c -- the 16 ANSI colours on screens of every depth. */
#include "palette.h"

/* ibmcon's unit-1 pen order swaps 1<->7 and 9<->15 against ANSI order. */
int Palette_IbmconToAnsi(int index)
{
    switch (index)
    {
        case 1:  return 7;
        case 7:  return 1;
        case 9:  return 15;
        case 15: return 9;
        default: return index;
    }
}

static int ibmconIndex(int ansi)
{
    switch (ansi)
    {
        case 1:  return 7;
        case 7:  return 1;
        case 9:  return 15;
        case 15: return 9;
        default: return ansi;
    }
}

static ULONG rgb4_to_rgb32(UWORD c)
{
    ULONG r = (c >> 8) & 0xF, g = (c >> 4) & 0xF, b = c & 0xF;

    return (r * 17) << 16 | (g * 17) << 8 | (b * 17);
}

static UWORD rgb32_to_rgb4(ULONG c)
{
    ULONG r = ((c >> 16 & 0xFF) * 15 + 127) / 255;
    ULONG g = ((c >> 8 & 0xFF) * 15 + 127) / 255;
    ULONG b = ((c & 0xFF) * 15 + 127) / 255;

    return (UWORD)(r << 8 | g << 4 | b);
}

void Palette_EnsureAnsi(struct PrefsStruct *p)
{
    int i;

    if (p->ansiMagic == PREFS_ANSI_MAGIC)
        return;
    for (i = 0; i < 16; i++)
        p->ansiRGB[i] = rgb4_to_rgb32(p->color[ibmconIndex(i)]);
    p->ansiMagic = PREFS_ANSI_MAGIC;
}

void Palette_SyncRGB4(struct PrefsStruct *p)
{
    int i;

    for (i = 0; i < 16; i++)
        p->color[ibmconIndex(i)] = rgb32_to_rgb4(p->ansiRGB[i]);
}

BOOL Palette_AnsiPens(UWORD depth, UBYTE pens[16])
{
    int i;

    if (depth < 5)
        return FALSE;
    for (i = 0; i < 8; i++)
    {
        pens[i]     = (UBYTE)(8 + i);
        pens[i + 8] = (UBYTE)(20 + i);
    }
    return TRUE;
}
