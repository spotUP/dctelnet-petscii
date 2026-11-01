/* --- the built-in renderer's terminal: vtcon parses, retro32-term draws ---
 *
 * Included by DCTelnet.c after term-engine.c, whose drawing it uses: the
 * glyph writer, the cell-aligned fill and scroll, the cursor flip and the
 * covered/RastPort switch stay exactly as they were. What changed is the
 * parser: bytes from the BBS go to vtcon's engine (third_party/vtcon, the
 * pcansi personality: ANSI.SYS / BBS art), which keeps the cell grid and
 * tells this file what to draw:
 *
 * - damage: cells whose content changed, drawn from the grid, one glyph
 *   each through term_glyph() with the cell's resolved pens;
 * - scroll: rows that moved, one term_rect_scroll(), the vacated rows in
 *   the default background (the engine's scroll contract);
 * - reply: DSR / DA answers, sent to the BBS.
 *
 * The engine orders every scroll before the damage that follows it, so a
 * damaged cell is drawn at once from the grid as it is now.
 *
 * term-engine.c's own parser (term_feed) is no longer called. Its cursor
 * position (cur_x, cur_y) is kept equal to the engine's after every write,
 * because the cursor flip and the RastPort glyph runs read it.
 */
#include "vtengine.h"
#include "glyphmap.h"
#include "vtpersonality.h"
#include "vtpens.h"
#include "vtinput.h"
#include "glyphraster.h"
#include "screenfont.h"

/* The glyph for a code point the font cannot show: the font's '?', one cell. */
#define VT_FALLBACK_GLYPH ((UBYTE)'?')

static vt_term *vtTerm;

/* The encoding of the terminal's font (VtProbeFont) and the one the glyph map
 * is asked for (Vt_GlyphEncoding: CP437 sessions stay byte pass-through). */
static enum vt_font_enc vtFontEnc;
static enum vt_font_enc vtGlyphEnc;

/* MA4: how many lines the view is scrolled back into the engine's scrollback;
 * 0 is the live grid. Grid row y shows engine row y - vtScrollOff. */
static int vtScrollOff;

/* Engine scrollback lines: on in a first run's prefs (Prefs_FirstRunState());
 * with the preference off the engine allocates none. */
static int VtScrollbackCap(void)
{
    if (!(prefs.State & APP_SCROLLBACK_ENABLED))
        return 0;
    return prefs.nScrollbackLines > 32767UL ? 32767 : (int)prefs.nScrollbackLines;
}

/* MA2-9: on a screen whose bitmap is deeper than 8 bits (RTG true or high
 * colour) a 256-colour or RGB cell gets a pen of its own colour from
 * ObtainBestPen(), cached per RGB in slots 16-255 of the engine's pen map.
 * Every other screen, the default colours and c < 16 keep Vt_PenFor() and
 * never call it (the 68000 path). */
#define VT_TC_MAX 240
static struct { ULONG rgb; LONG pen; } vtTc[VT_TC_MAX];
static int vtTcN, vtTcLast;
static BOOL vtTrueColour;
#ifdef _DEBUG
static LONG dbgTcCalls, dbgTcLastPen = 12345;   /* MA2-9: ObtainBestPen calls and its last answer */
#endif

/* The pen to draw colour c with. */
static UBYTE VtPenFor(vt_color c)
{
    ULONG rgb;
    LONG pen;
    int i;

    if (!vtTrueColour || c < 16 || c == VT_COLOR_DEFAULT || c == VT_COLOR_DEFAULT_BG)
        return Vt_PenFor(c, prefs.AnsiColors);
    rgb = c < 256 ? Vt_Xterm256Rgb((int)c) : (ULONG)VT_RGB_OF(c);
    if (vtTcN && vtTc[vtTcLast].rgb == rgb)
        return (UBYTE)(16 + vtTcLast);
    for (i = 0; i < vtTcN; i++) {
        if (vtTc[i].rgb == rgb) {
            vtTcLast = i;
            return (UBYTE)(16 + i);
        }
    }
    if (vtTcN >= VT_TC_MAX)
        return Vt_PenFor(c, prefs.AnsiColors);
    pen = ObtainBestPen(scr->ViewPort.ColorMap,
                        ((rgb >> 16) & 0xFF) * 0x01010101UL,
                        ((rgb >> 8) & 0xFF) * 0x01010101UL,
                        (rgb & 0xFF) * 0x01010101UL,
                        OBP_Precision, PRECISION_EXACT, OBP_FailIfBad, TRUE, TAG_END);
#ifdef _DEBUG
    dbgTcCalls++;
    dbgTcLastPen = pen;
#endif
    if (pen < 0)
        return Vt_PenFor(c, prefs.AnsiColors);
    vtTc[vtTcN].rgb = rgb;
    vtTc[vtTcN].pen = pen;
    term_set_pen((UBYTE)(16 + vtTcN), (UBYTE)pen);
    vtTcLast = vtTcN;
    return (UBYTE)(16 + vtTcN++);
}

/* Draw cells x0 <= x < x1 of rows y0 <= y < y1 from the grid. */
static void VtDraw(void *user, int x0, int y0, int x1, int y1)
{
    int x, y, n;
#ifdef _DEBUG
    if (vtScrollOff > 0)
        dbgScrollDraws++;           /* sentinel MA4-5: a scrollback view was drawn */
#endif
    /* vt_new() draws its blank screen before it returns the terminal:
     * term_init_area() has cleared the area already. */
    if (!vtTerm)
        return;
    for (y = y0; y < y1; y++) {
        const vt_cell *c = vt_row(vtTerm, y - vtScrollOff, &n);
        if (!c)
            continue;
        cur_y = (WORD)y;
        term_sync_row_base();
        for (x = x0; x < x1 && x < n; x++) {
            vt_color fg, bg;
            vt_u32 ch = c[x].ch;
            vt_cell cell = c[x];
            UBYTE code;
            /* Blink is never animated (no timer; the owner's decision, MA1-7
             * exemption). With iCE colours on (BLINK_ICE) the engine turns
             * blink on a dark background into the bright background; with
             * them off (BLINK_STEADY) the flag is cleared here, so the cell is
             * drawn as it was sent. */
            if (prefs.Blink == BLINK_STEADY)
                cell.attr &= (vt_attr)~VT_ATTR_BLINK;
            vt_resolve_colors(vtTerm, &cell, &fg, &bg);
            term_pen_fg = VtPenFor(fg);
            term_pen_bg = VtPenFor(bg);
            atr_under = (c[x].attr & VT_ATTR_UNDERLINE) != 0;
            if (c[x].width == 0) {
                /* the second cell of a wide character: the font's cells are
                 * one wide, the first cell drew the fallback glyph */
                code = (UBYTE)' ';
            } else if (c[x].width > 1) {
                code = VT_FALLBACK_GLYPH;
            } else if (ch >= 0x20 && ch < 0x7F) {
                code = (UBYTE)ch;
            } else {
                /* A CP437 session (PC ANSI) is a byte pass-through: the byte
                 * the BBS sent is the font's character, the font decides
                 * the look. A Unicode session maps the code point into the
                 * font's own encoding (vtGlyphEnc, probed from the font);
                 * what the font lacks (box, blocks, diagonals) is drawn. */
                vt_glyph g = vt_map_glyph(ch, vtGlyphEnc);
                if (g.kind != VT_GLYPH_FONT) {
                    UWORD rows[8];
                    GlyphRaster_Draw(g, 1 << term_xs, 8, rows);
                    term_glyph_bits((WORD)x, rows);
                    continue;
                }
                code = g.code;
            }
            term_glyph((WORD)x, code);
        }
        /* a scrollback line from before a resize can be narrower than the grid */
        if (x < x1) {
            vt_cell blank;
            vt_color bfg, bbg;
            memset(&blank, 0, sizeof(blank));
            blank.ch = ' ';
            blank.width = 1;
            blank.fg = VT_COLOR_DEFAULT;
            blank.bg = VT_COLOR_DEFAULT;
            vt_resolve_colors(vtTerm, &blank, &bfg, &bbg);
            term_pen_fg = VtPenFor(bfg);
            term_pen_bg = VtPenFor(bbg);
            atr_under = 0;
            for (; x < x1; x++)
                term_glyph((WORD)x, (UBYTE)' ');
        }
    }
    atr_under = 0;
}

static void VtScroll(void *user, int top, int bottom, int n)
{
    /* pcansi's default background is colour 0 */
    term_rect_scroll(0, (WORD)top, term_cols, (WORD)(bottom - top), 0, (WORD)-n, 0);
}

/* The screen's layers while the engine draws straight into its bitmap.
 * It does only while no layer lies over the terminal (a requester, another
 * DCTelnet window, an open menu): such a layer owns those pixels. Intuition
 * takes the screen's LayerInfo to make a layer, so holding it from the look
 * to the end of the drawing (the blitter's too) keeps a menu or window from
 * opening in between and being drawn over. */
static BOOL vtLayersHeld;

/* Is a layer in front of the terminal window over its inner area? The
 * caller holds the screen's LayerInfo. */
static BOOL VtCovered(void)
{
    struct Layer *l;
    WORD x0 = win->LeftEdge + win->BorderLeft;
    WORD y0 = win->TopEdge + win->BorderTop;
    WORD x1 = win->LeftEdge + win->Width - win->BorderRight - 1;
    WORD y1 = win->TopEdge + win->Height - win->BorderBottom - 1;

    for (l = scr->LayerInfo.top_layer; l && l != win->WLayer; l = l->back)
        if (l->bounds.MinX <= x1 && l->bounds.MaxX >= x0
         && l->bounds.MinY <= y1 && l->bounds.MaxY >= y0)
            return TRUE;
    return FALSE;
}

/* Before drawing. Covered, the engine draws through the window's RastPort,
 * whose layer clips, and the lock is given back at once; otherwise it draws
 * straight into the bitmap and the layers stay locked until
 * VtLayersUnlock(). Without layers.library: the RastPort. */
static void VtLayersLock(void)
{
    BOOL covered = TRUE;

    if (!scr || !win)
        return;
    if (LayersBase) {
        LockLayerInfo(&scr->LayerInfo);
        covered = VtCovered();
    }
    term_covered(covered);
    if (covered && LayersBase)
        UnlockLayerInfo(&scr->LayerInfo);
    else if (!covered)
        vtLayersHeld = TRUE;
}

static void VtLayersUnlock(void)
{
    if (!vtLayersHeld)
        return;
    term_blit_sync();       /* the blitter is done with the planes too */
    vtLayersHeld = FALSE;
    UnlockLayerInfo(&scr->LayerInfo);
}

/* An answer to the BBS, in the middle of a write: sent with the layers
 * unlocked (a send that waits would hold up every window on the screen),
 * looked at again after. */
static void VtReply(void *user, const vt_u8 *buf, long len)
{
    BOOL held = vtLayersHeld;

    VtLayersUnlock();
#ifdef _DEBUG
    dbgReplyBytes += len;
#endif
    send_data((const UBYTE *)buf, len);
    if (held)
        VtLayersLock();
}

/* After the engine has drawn: the cursor where the engine has it. */
static void VtSyncCursor(void)
{
    int x, y;
    vt_cursor(vtTerm, &x, &y);
    cur_x = (WORD)x;
    cur_y = (WORD)y;
    term_sync_row_base();
    cursor_visible = vtScrollOff == 0 && (vt_modes(vtTerm) & VT_MODE_CURSOR_VISIBLE) != 0;
}

/* MA2-14: per connection, whether the host turned out to speak UTF-8. */
static struct VtUtf8Sniffer vtSniff;

/* The personality and character set the preferences ask for (Terminal >
 * Terminal Type, Character Set): at open, and when either menu changes. */
static void VtApplyPersonality(void)
{
    struct VtPersonality pers = Vt_PersonalityForConnection(&prefs, &vtSniff);

    vtGlyphEnc = Vt_GlyphEncoding(pers.charset, vtFontEnc);
    if (!vtTerm)
        return;
    vt_set_personality(vtTerm, pers.personality);
    vt_set_charset(vtTerm, pers.charset);
}

static void VtSniffReset(void)
{
    Vt_Utf8SnifferReset(&vtSniff);
}

/* MA3-2/3: the window title a program set (OSC 0/2), as Latin-1; none (or the
 * session over) gives DCTelnet's own text back. */
static char vtTitleShown[256];  /* what the title bar holds (Latin-1) */
static char vtTitleSeen[256];   /* the engine's title as last looked at (UTF-8) */

static void VtSetTitle(const char *utf8)
{
    char text[sizeof(vtTitleShown)];
    const char *screenTitle;
    ULONG keepScreenTitle;

    Vt_TitleText(utf8, MainWindowTitle, text, sizeof(text));
    if (!win || strcmp(text, vtTitleShown) == 0)
        return;
    strcpy(vtTitleShown, text);
#ifdef _DEBUG
    if (utf8[0])                    /* the restore to DCTelnet's own title must not overwrite it */
        strncpy((char *)dbgTitle, text, sizeof(dbgTitle) - 1);     /* MA3-2: the bytes the window got */
#endif
    screenTitle = Vt_ScreenTitleFor(win->Flags, WFLG_BACKDROP, vtTitleShown);
    keepScreenTitle = 0xFFFFFFFFUL;         /* -1: the screen title stays */
    SetWindowTitles(win, (UBYTE *)vtTitleShown,
                    screenTitle ? (UBYTE *)screenTitle : (UBYTE *)(size_t)keepScreenTitle);
    if (screenTitle)
        LEDs();                             /* the bar redraw erased the activity LEDs */
}

static void VtTitle(void *user, const char *utf8)
{
    strncpy(vtTitleSeen, utf8, sizeof(vtTitleSeen) - 1);
    VtSetTitle(utf8);
}

/* After a write: a reset (ESC c) clears the engine's title without a callback. */
static void VtTitleSync(void)
{
    const char *t = vt_title(vtTerm);

    if (strcmp(t, vtTitleSeen) == 0)
        return;
    strncpy(vtTitleSeen, t, sizeof(vtTitleSeen) - 1);
    VtSetTitle(t);
}

/* MA3-3: back to DCTelnet's own title (session over); the engine's old title
 * is not applied again by the next write. */
static void VtTitleRestore(void)
{
    if (vtTerm)
        strncpy(vtTitleSeen, vt_title(vtTerm), sizeof(vtTitleSeen) - 1);
    VtSetTitle("");
}

/* MA3-4..6: the mouse as the program on the far side asked for it. */
static struct VtMouse vtMouse;

/* IDCMP_MOUSEMOVE only while ?1002 or ?1003 is on (the built-in renderer has
 * no selection to drag). */
static void VtSyncMouseIdcmp(void)
{
    ULONG want;

    if (!win || !vtTerm)
        return;
    want = Vt_IdcmpFor(win->IDCMPFlags, IDCMP_MOUSEMOVE, vtTerm, FALSE);
    if (want != win->IDCMPFlags)
        ModifyIDCMP(win, want);
}

/* TRUE when the program owns the mouse (a mouse mode is on, Shift not held):
 * the event is for it and selecting must not start. */
static BOOL VtMouseOwned(BOOL shift)
{
    return vtTerm && Vt_MouseOwned(vtTerm, shift);
}

/* A button or wheel event at cell x, y (from 0): the report goes out. */
static void VtMouseButton(int button, BOOL down, int x, int y, int mods)
{
    UBYTE out[32];
    int n;

    if (!vtTerm)
        return;
    n = Vt_MouseButton(&vtMouse, vtTerm, button, down, x, y, mods, out);
#ifdef _DEBUG
    if (n > 0) dbgMouseOut++;
#endif
    if (n > 0)
        VtReply(NULL, out, n);
}

static void VtMouseMove(int x, int y, int mods)
{
    UBYTE out[32];
    int n;

    if (!vtTerm)
        return;
    n = Vt_MouseMove(&vtMouse, vtTerm, x, y, mods, out);
    if (n > 0)
        VtReply(NULL, out, n);
}

static void VtClose(void);

/* The encoding of the terminal's font, from its glyph 0xC4: a line in an IBM
 * (CP437) font, a letter in an Amiga (Latin-1) one. After the font is
 * extracted (term_init_area, term_set_font). */
static void VtProbeFont(void)
{
    UWORD rows[8];
    int r;

    for (r = 0; r < 8; r++)
        rows[r] = term_xs == 3 ? (UWORD)((UWORD)font8[(0xC4 << 3) + r] << 8) : font16[(0xC4 << 3) + r];
    vtFontEnc = ScreenFont_GlyphIsHLine(rows, 8, 1 << term_xs) ? VT_ENC_CP437 : VT_ENC_LATIN1;
    vtGlyphEnc = Vt_GlyphEncoding(Vt_PersonalityForConnection(&prefs, &vtSniff).charset, vtFontEnc);
}

/* A new terminal for the grid term_init_area() has just laid out (and
 * cleared). 0 when it is ready. */
static int VtOpen(void)
{
    static const vt_callbacks cb = { VtDraw, VtScroll, VtReply, NULL, VtTitle, NULL, NULL };

    vtScrollOff = 0;
    VtProbeFont();
    VtClose();      /* vt_new() draws through VtDraw(): vtTerm must not be the freed one */
    vtTrueColour = FALSE;
    if (scr && scr->RastPort.BitMap) {
        struct BitMap *bm = scr->RastPort.BitMap;
        ULONG depth = ((struct Library *)GfxBase)->lib_Version >= 39 ? GetBitMapAttr(bm, BMA_DEPTH) : bm->Depth;

        vtTrueColour = depth > 8;
    }
    vtTerm = vt_new(term_cols, term_rows, VtScrollbackCap(), &cb, NULL);
    if (!vtTerm)
        return 1;
    VtApplyPersonality();
    Vt_MouseReset(&vtMouse);
    vtTitleSeen[0] = vtTitleShown[0] = 0;
    VtSyncMouseIdcmp();
    VtSyncCursor();
    return 0;
}

/* Every cell again from the grid, after a preference that changes how the
 * cells look (Terminal > iCE Colours). */
static void VtRedraw(void)
{
    if (!vtTerm)
        return;
    VtLayersLock();
    cursor_hide();
    VtDraw(NULL, 0, 0, term_cols, term_rows);
    term_flush();
    VtSyncCursor();
    cursor_show();
    VtLayersUnlock();
}

#ifdef _DEBUG
/* MA1-10 / MA-rig: the sentinels and the engine's grid to a file (name), as the screen shows it
 * (scrollback offset applied): counters, window metrics, the title bytes, CRC32 over ch lo, ch hi,
 * fg, bg, attr of the live grid (the same sum as test_ma1_9), the pens of the non-blank cells,
 * then the displayed rows as text. once: the VtClose dump runs a single time. */
/* cybergraphics.library ReadRGBPixel (LVO -108), declared here: the vbcc NDK has no cybergraphics headers */
static ULONG DbgReadRGBPixel(__reg("a6") struct Library *, __reg("a1") struct RastPort *, __reg("d0") UWORD, __reg("d1") UWORD) = "\tjsr\t-108(a6)";

static void VtDumpGrid(const char *name, BOOL once)
{
    struct Library *cgx = NULL;
    BPTR f;
    ULONG crc = 0xFFFFFFFFUL;
    int x, y, n, k, j, np = 0;

    static BOOL dumped = FALSE;

    if (once) {
        if (dumped || (!vtTerm && !dbgConWriteCount && !dbgVtWriteCount))
            return;                 /* already written, or the early close before any write */
        dumped = TRUE;
    }
    f = Open((STRPTR)name, MODE_NEWFILE);
    if (once)
        PutStr(f ? (STRPTR)"vtgrid: dump file open\n" : (STRPTR)"vtgrid: Open dump FAILED\n");
    if (!f)
        return;
    if (!vtTerm) {
        FPrintf(f, (STRPTR)"vtwrite %ld\nconwrite %ld\nvt none\n", (LONG)dbgVtWriteCount,
                (LONG)dbgConWriteCount);
        Close(f);
        return;
    }
    for (y = 0; y < vt_rows(vtTerm); y++) {
        const vt_cell *row = vt_row(vtTerm, y, &n);

        for (x = 0; x < n; x++) {
            UBYTE b[5];
            b[0] = (UBYTE)(row[x].ch & 255); b[1] = (UBYTE)(row[x].ch >> 8);
            b[2] = (UBYTE)row[x].fg; b[3] = (UBYTE)row[x].bg; b[4] = (UBYTE)row[x].attr;
            for (k = 0; k < 5; k++) {
                crc ^= b[k];
                for (j = 0; j < 8; j++)
                    crc = (crc >> 1) ^ (0xEDB88320UL & (0UL - (crc & 1)));
            }
        }
    }
    crc ^= 0xFFFFFFFFUL;
    FPrintf(f, (STRPTR)"vtwrite %ld\nconwrite %ld\nrows %ld\ncols %ld\ncrc %08lx\n",
            (LONG)dbgVtWriteCount, (LONG)dbgConWriteCount, (LONG)vt_rows(vtTerm), (LONG)term_cols,
            (LONG)crc);
    FPrintf(f, (STRPTR)"scrolldraws %ld\nscrolloff %ld\nscrollback %ld\nutf8conv %ld\n",
            (LONG)dbgScrollDraws, (LONG)vtScrollOff, (LONG)vt_scrollback_lines(vtTerm),
            (LONG)dbgUtf8Conv);
    FPrintf(f, (STRPTR)"modes %08lx\nmouseev %ld owned %ld out %ld replybytes %ld pix %ld,%ld cell r%ld c%ld\n", (LONG)vt_modes(vtTerm), (LONG)dbgMouseEvents, (LONG)dbgMouseOwned, (LONG)dbgMouseOut, (LONG)dbgReplyBytes, (LONG)dbgMouseX, (LONG)dbgMouseY, (LONG)dbgMouseRow, (LONG)dbgMouseCol);
    FPrintf(f, (STRPTR)"title");
    for (k = 0; dbgTitle[k]; k++)
        FPrintf(f, (STRPTR)" %02lx", (LONG)dbgTitle[k]);
    FPrintf(f, (STRPTR)"\n");
    if (win)
        FPrintf(f, (STRPTR)"win %ld x %ld border L%ld R%ld T%ld B%ld inner %ld x %ld cells %ld x %ld px (term_x0 %ld term_y0 %ld xs %ld) screen depth %ld\n",
                (LONG)win->Width, (LONG)win->Height, (LONG)win->BorderLeft, (LONG)win->BorderRight,
                (LONG)win->BorderTop, (LONG)win->BorderBottom,
                (LONG)(win->Width - win->BorderLeft - win->BorderRight),
                (LONG)(win->Height - win->BorderTop - win->BorderBottom),
                (LONG)(term_cols << term_xs), (LONG)(term_rows << 3), (LONG)term_x0, (LONG)term_y0,
                (LONG)term_xs, (LONG)win->WScreen->RastPort.BitMap->Depth);
    for (y = 0; y < vt_rows(vtTerm); y++) {
        const vt_cell *row = vt_row(vtTerm, y - vtScrollOff, &n);

        for (x = 0; row && x < n && np < 24; x++) {
            vt_color fg, bg;
            vt_cell c = row[x];

            if (c.ch == ' ' && c.bg == VT_COLOR_DEFAULT)
                continue;
            vt_resolve_colors(vtTerm, &c, &fg, &bg);
            if (c.fg != VT_COLOR_DEFAULT || c.bg != VT_COLOR_DEFAULT) {
                FPrintf(f, (STRPTR)"pen y%ld x%ld ch %lx fgraw %lx fgpen %ld bgpen %ld\n", (LONG)y, (LONG)x,
                        (LONG)c.ch, (LONG)c.fg, (LONG)Vt_PenFor(fg, prefs.AnsiColors),
                        (LONG)Vt_PenFor(bg, prefs.AnsiColors));
                np++;
            }
        }
    }
    for (y = 0; y < vt_rows(vtTerm); y++) {
        const vt_cell *row = vt_row(vtTerm, y - vtScrollOff, &n);

        for (x = 0; row && x < n; x++)
            FPutC(f, (row[x].ch >= 32 && row[x].ch < 127) ? (LONG)row[x].ch : (LONG)'.');
        FPutC(f, (LONG)'\n');
    }
    for (y = 0; y < vt_rows(vtTerm); y++) {          /* MA2-12: real cell codepoints of non-ASCII cells */
        const vt_cell *row = vt_row(vtTerm, y - vtScrollOff, &n);

        for (x = 0; row && x < n; x++)
            if (row[x].ch >= 127)
                FPrintf(f, (STRPTR)"cp y%ld x%ld %lx\n", (LONG)y, (LONG)x, (LONG)row[x].ch);
    }
    /* MA2-9 / MA2-10: true-colour flag, pens obtained, and the pixels of the first RGB-fg cell */
    FPrintf(f, (STRPTR)"tc vtTrueColour %ld vtTcN %ld obtaincalls %ld lastpen %ld\n", (LONG)vtTrueColour, (LONG)vtTcN,
            dbgTcCalls, dbgTcLastPen);
    {
        int n0;
        const vt_cell *r0 = vt_row(vtTerm, 0, &n0);
        if (r0 && n0 > 0)
            FPrintf(f, (STRPTR)"cell00 ch %lx fg %lx bg %lx\n", (LONG)r0[0].ch, (LONG)r0[0].fg, (LONG)r0[0].bg);
    }
    for (k = 0; k < vtTcN && k < 8; k++)
        FPrintf(f, (STRPTR)"tcslot %ld rgb %06lx pen %ld\n", (LONG)k, (LONG)vtTc[k].rgb, (LONG)vtTc[k].pen);
    if (win && scr) {
        BOOL found = FALSE;
        cgx = OpenLibrary((STRPTR)"cybergraphics.library", 40);
        FPrintf(f, (STRPTR)"cgx %lx\n", (LONG)cgx);
        for (y = 0; y < vt_rows(vtTerm) && !found; y++) {
            const vt_cell *row = vt_row(vtTerm, y, &n);
            for (x = 0; row && x < n && !found; x++) {
                int px, py, np2 = 0;
                LONG seen[8];
                if (row[x].ch == ' ' || row[x].fg < 256 || row[x].fg == VT_COLOR_DEFAULT)
                    continue;
                found = TRUE;
                FPrintf(f, (STRPTR)"rgbcell y%ld x%ld ch %lx fgraw %lx base px %ld,%ld\n", (LONG)y, (LONG)x,
                        (LONG)row[x].ch, (LONG)row[x].fg, (LONG)(term_x0 + (x << term_xs)), (LONG)(term_y0 + y * 8));
                for (py = 0; py < 8; py++)
                    for (px = 0; px < (8 << term_xs); px++) {
                        LONG pen = ReadPixel(&scr->RastPort, term_x0 + (x << term_xs) + px, term_y0 + y * 8 + py);
                        LONG rgbp = cgx ? (LONG)DbgReadRGBPixel(cgx, &scr->RastPort, (UWORD)(term_x0 + (x << term_xs) + px), (UWORD)(term_y0 + y * 8 + py)) : -1L;
                        ULONG rgb[3];
                        int q, dup = 0;
                        for (q = 0; q < np2; q++)
                            if (seen[q] == (pen ^ rgbp))
                                dup = 1;
                        if (dup || np2 >= 8)
                            continue;
                        seen[np2++] = pen ^ rgbp;
                        GetRGB32(scr->ViewPort.ColorMap, (ULONG)pen, 1, rgb);
                        FPrintf(f, (STRPTR)"pixel dx%ld dy%ld pen %ld rgb32 %08lx %08lx %08lx rgbpixel %08lx\n", (LONG)px, (LONG)py,
                                pen, (LONG)rgb[0], (LONG)rgb[1], (LONG)rgb[2],
                                rgbp);
                    }
            }
        }
    }
    if (cgx)
        CloseLibrary(cgx);
    FPrintf(f, (STRPTR)"ansi");
    for (k = 0; k < 16; k++)
        FPrintf(f, (STRPTR)" %03lx", (LONG)prefs.AnsiColors[k]);
    FPrintf(f, (STRPTR)"\n");
    Close(f);
}
#endif

static void VtClose(void)
{
#ifdef _DEBUG
    VtDumpGrid("T:vtgrid.txt", TRUE);
#endif
    while (vtTcN > 0)
        ReleasePen(scr->ViewPort.ColorMap, (ULONG)vtTc[--vtTcN].pen);
    vtTcLast = 0;
    if (vtTerm) {
        vt_free(vtTerm);
        vtTerm = NULL;
    }
}

/* Scroll the view delta lines back (> 0) or forward (< 0) through the engine's
 * scrollback, clamped to what it holds; no scrollback on the alt screen (vim).
 * Returns the offset now shown. */
static int VtScrollView(int delta)
{
    int want = vtScrollOff + delta;
    int max = (vtTerm && !(vt_modes(vtTerm) & VT_MODE_ALT_SCREEN)) ? vt_scrollback_lines(vtTerm) : 0;

    if (want > max)
        want = max;
    if (want < 0)
        want = 0;
    if (!vtTerm || want == vtScrollOff)
        return vtScrollOff;
    vtScrollOff = want;
    VtLayersLock();
    cursor_hide();
    VtDraw(NULL, 0, 0, term_cols, term_rows);
    term_flush();
    VtSyncCursor();
    cursor_show();
    VtLayersUnlock();
#ifdef _DEBUG
    VtDumpGrid("T:vtscroll.txt", FALSE);    /* MA4-5: what the scrollback view shows */
#endif
    return vtScrollOff;
}

/* A key or wheel event for the scrollback: Shift+Page Up/Down scroll a page
 * (plain Page Up still goes to the host: less and vim use it), the NewMouse
 * wheel scrolls 3 lines unless a program owns the mouse. TRUE when taken. */
static BOOL VtScrollKey(UWORD rawkey, BOOL shift)
{
    int page = vtTerm ? vt_rows(vtTerm) - 1 : 0;
    int wheel = Vt_WheelButton(rawkey);

    if (!vtTerm || VtScrollbackCap() == 0)
        return FALSE;
    if (rawkey == 0x48 && shift)
        VtScrollView(page);
    else if (rawkey == 0x49 && shift)
        VtScrollView(-page);
    else if (wheel >= 0 && !VtMouseOwned(shift))
        VtScrollView(wheel == 64 ? 3 : -3);
    else
        return FALSE;
    return TRUE;
}

/* Bytes from the BBS (or DCTelnet's own messages): parse, draw what
 * changed, put the cursor back. Drawing happens with the cursor hidden. */
static void VtWrite(const UBYTE *data, long len)
{
#ifdef _DEBUG
    dbgVtWriteCount++;
#endif
    if (vtScrollOff)
        VtScrollView(-vtScrollOff);     /* new output: back to the live grid */
    VtLayersLock();
    cursor_hide();
    /* Unix host, Character Set not UTF-8: the first valid UTF-8 sequence moves
     * the engine to UTF-8 for the rest of the connection; prefs stay as they are.
     * A trailing incomplete prefix is held back until the next write decides it. */
    if (Vt_Utf8SnifferActive(&prefs) || vtSniff.nheld) {
        UBYTE pre[3];
        long npre, ndata;

        if (Vt_Utf8SnifferFeed(&vtSniff, &prefs, data, len, pre, &npre, &ndata))
            vt_set_charset(vtTerm, VT_CS_UTF8);
        if (npre)
            vt_write(vtTerm, pre, npre);
        vt_write(vtTerm, data, ndata);
    } else
        vt_write(vtTerm, data, len);
    term_flush();
    VtSyncCursor();
    cursor_show();
    VtLayersUnlock();
    VtTitleSync();
#ifdef _DEBUG
    VtDumpGrid("T:vtlive.txt", FALSE);      /* the live grid after this write (rig only) */
#endif
    VtSyncMouseIdcmp();             /* ?1002 / ?1003 may have changed */
}

/* A prefix VtWrite held back is waiting to be drawn: the receive loop polls
 * instead of sleeping, so VtFlushHeld() runs as soon as the socket is idle. */
static BOOL VtHeldPending(void)
{
    return vtSniff.nheld != 0;
}

/* The receive loop found no more pending data: a prefix VtWrite held back
 * waits for nothing, so it is drawn as the current character set. */
static void VtFlushHeld(void)
{
    UBYTE pre[3];
    long n;

    if (!vtSniff.nheld)
        return;
    if (vtScrollOff)
        VtScrollView(-vtScrollOff);
    n = Vt_Utf8SnifferTakeHeld(&vtSniff, pre);
    VtLayersLock();
    cursor_hide();
    vt_write(vtTerm, pre, n);
    term_flush();
    VtSyncCursor();
    cursor_show();
    VtLayersUnlock();
}
