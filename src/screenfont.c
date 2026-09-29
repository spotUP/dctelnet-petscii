/* src/screenfont.c -- the terminal and screen font on square-pixel modes. */
#include "screenfont.h"

static BOOL same_name(const char *a, const char *b)
{
    for (; *a && *b; a++, b++)
    {
        char x = (*a >= 'A' && *a <= 'Z') ? (char)(*a + 32) : *a;
        char y = (*b >= 'A' && *b <= 'Z') ? (char)(*b + 32) : *b;
        if (x != y) return FALSE;
    }
    return *a == *b;
}

/* One face in two drawings: for the Amiga's tall pixels (PAL hires) and
 * for square ones (RTG, AGA hires interlaced). */
static const struct
{
    const char *tallName;
    UWORD       tallSize;
    const char *squareName;
    UWORD       squareSize;
} pairs[] =
{
    { "topaz.font", 8, TOPAZ_PRO_NAME, TOPAZ_PRO_SIZE },  /* Topaz Pro: topaz redrawn */
    { "IBM.font",   8, "IBM.font",     16 },              /* the VGA 8x16 IBM PC font */
};

void ScreenFont_ForMode(const char *fontname, UWORD fontsize,
                        UWORD resolutionX, UWORD resolutionY, BOOL squareFits,
                        struct ScreenFontChoice *out)
{
    BOOL known = resolutionX != 0 && resolutionY != 0;
    int i;

    out->name = fontname;
    out->size = fontsize;
    out->topazPro = same_name(fontname, TOPAZ_PRO_NAME);
    out->square = FALSE;
    if (!known)
        return;
    for (i = 0; i < (int)(sizeof(pairs) / sizeof(pairs[0])); i++)
    {
        BOOL tall = same_name(fontname, pairs[i].tallName) && fontsize == pairs[i].tallSize;
        BOOL square = same_name(fontname, pairs[i].squareName) && fontsize == pairs[i].squareSize;

        /* topaz and Topaz Pro: any size is the one face */
        if (i == 0)
        {
            tall = same_name(fontname, pairs[i].tallName);
            square = same_name(fontname, pairs[i].squareName);
        }
        if (!tall && !square)
            continue;
        if (resolutionX == resolutionY && squareFits)
        {
            out->name = pairs[i].squareName;
            out->size = pairs[i].squareSize;
            out->square = !square;
        }
        else
        {
            out->name = pairs[i].tallName;
            out->size = pairs[i].tallSize;
        }
        out->topazPro = same_name(out->name, TOPAZ_PRO_NAME);
        return;
    }
}

void ScreenFont_Grid(UWORD width, UWORD height, UWORD gzzWidth, UWORD gzzHeight, BOOL gzz,
                     UWORD cellX, UWORD cellY, UWORD *cols, UWORD *rows)
{
    UWORD w = gzz ? gzzWidth : width;
    UWORD h = gzz ? gzzHeight : height;

    *cols = cellX ? (UWORD)(w / cellX) : 1;
    *rows = cellY ? (UWORD)(h / cellY) : 1;
    if (*cols == 0) *cols = 1;
    if (*rows == 0) *rows = 1;
    if (*cols > SCREENFONT_MAX_COLS) *cols = SCREENFONT_MAX_COLS;
}

UWORD ScreenFont_MaxWindowWidth(UWORD columns, UWORD cellX, UWORD borders)
{
    return (UWORD)(columns * cellX + borders);
}

BOOL ScreenFont_TallPixels(UWORD resolutionX, UWORD resolutionY)
{
    return resolutionX != 0 && resolutionY > resolutionX;
}
