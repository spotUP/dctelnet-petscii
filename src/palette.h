/* src/palette.h -- the 16 ANSI colours on screens of every depth.
 *
 * Pure functions on struct PrefsStruct, unit-tested on the host.
 */
#ifndef PALETTE_H
#define PALETTE_H

#include "prefs.h"

/* Fill ansiRGB from the RGB4 color[] when the prefs predate it. */
void Palette_EnsureAnsi(struct PrefsStruct *p);

/* Rewrite color[] (RGB4, ibmcon order) from ansiRGB, for screens with 16 or
 * fewer colours, which still load RGB4 palettes. */
void Palette_SyncRGB4(struct PrefsStruct *p);

/* TRUE when a screen of this depth gives the ANSI colours pens of their own:
 * 32+ colours, pens 8-15 and 20-27 (UI pens 0-7 and the AGA pointer pens
 * 16-19 left alone), in ANSI order. FALSE: the ANSI colours share pens 0-15
 * with the UI, as before. */
BOOL Palette_AnsiPens(UWORD depth, UBYTE pens[16]);

#endif /* PALETTE_H */
