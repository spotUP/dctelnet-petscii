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

BOOL ScreenFont_WantsTopazPro(const char *fontname, UWORD resolutionX, UWORD resolutionY)
{
    return resolutionX != 0 && resolutionX == resolutionY && same_name(fontname, "topaz.font");
}
