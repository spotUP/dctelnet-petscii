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

#define TOPAZ_PRO_NAME "TopazPro.font"
#define TOPAZ_PRO_SIZE 16

#endif /* SCREENFONT_H */
