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

/* ibmcon's unit-1 order <-> ANSI order (the swap is its own inverse). */
int Palette_IbmconToAnsi(int index);

/* The standard ANSI (VGA) colours as 0x00RRGGBB, in ANSI order: what the
 * palette editor's Default button restores. */
void Palette_Default(ULONG rgb[16]);

/* The colour's name in ANSI order ("Black" ... "Bright White"). */
const char *Palette_Name(int ansiIndex);

/* One channel (0 red, 1 green, 2 blue) of a 0x00RRGGBB colour, and the
 * colour with that channel set to value (0-255). */
UBYTE Palette_Channel(ULONG rgb, int channel);
ULONG Palette_WithChannel(ULONG rgb, int channel, UBYTE value);

/* On a true-colour screen the terminal's pixels hold colours, so the
 * palette editor recolours them: every pixel of shown[sel] becomes newRGB.
 * Safe only when no other ANSI colour shares either value -- else the two
 * colours' pixels would merge for good. */
BOOL Palette_SafeRecolour(const ULONG shown[16], int sel, ULONG newRGB);

#endif /* PALETTE_H */
