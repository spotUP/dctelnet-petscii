/* src/screenfont.h -- the terminal and screen font on square-pixel modes. */
#ifndef SCREENFONT_H
#define SCREENFONT_H

#include <exec/types.h>

/* Topaz was drawn for the Amiga's tall non-square pixels. On a mode with
 * square pixels (resolution ticks x == y: every RTG mode, AGA hires
 * interlaced) it looks squashed, and Topaz Pro -- topaz redrawn for square
 * pixels, 8x16 -- replaces it. A font the user chose is never swapped.
 * resolutionX/Y are DisplayInfo.Resolution (0 when unknown). */
BOOL ScreenFont_WantsTopazPro(const char *fontname, UWORD resolutionX, UWORD resolutionY);

/* The text grid a terminal window holds, as ibmcon.device measures it
 * (MeasureGrid): the drawable area -- the inner area of a GimmeZeroZero
 * window, which the Workbench window is -- divided by the font cell, at
 * least 1 x 1 and at most SCREENFONT_MAX_COLS columns. Pass the window's
 * Width/Height, GZZWidth/GZZHeight and whether WFLG_GIMMEZEROZERO is set. */
#define SCREENFONT_MAX_COLS 199
void ScreenFont_Grid(UWORD width, UWORD height, UWORD gzzWidth, UWORD gzzHeight, BOOL gzz,
                     UWORD cellX, UWORD cellY, UWORD *cols, UWORD *rows);

#define TOPAZ_PRO_NAME "TopazPro.font"
#define TOPAZ_PRO_SIZE 16

#endif /* SCREENFONT_H */
