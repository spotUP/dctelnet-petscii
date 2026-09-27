/* src/site_prefs.c -- per-Address-Book-entry settings (issue #10). */
#include <stddef.h>
#include "site_prefs.h"

/* Flags whose change needs the screen reopened (OpenAppScreen reads them).
 * FLAG_PETSCII_MODE is not one: the C64 display is up only during a
 * connection, and DCTelnet.c switches it with the connection itself. */
#define SCREEN_FLAGS  (FLAG_USE_WORKBENCH | FLAG_HIDE_TITLEBAR)

/* Flags whose change needs the windows reopened (OpenDisplay reads them). */
#define WINDOW_FLAGS  (FLAG_PACKET_WINDOW | FLAG_USE_XEM_LIBRARY | FLAG_TOOL_BAR | FLAG_JUMP_SCROLL)

static void copy_geometry(struct PrefsStruct *to, const struct PrefsStruct *from)
{
    to->win_left        = from->win_left;
    to->win_top         = from->win_top;
    to->win_width       = from->win_width;
    to->win_height      = from->win_height;
    to->sb_left         = from->sb_left;
    to->sb_top          = from->sb_top;
    to->sb_width        = from->sb_width;
    to->sb_height       = from->sb_height;
    to->toolBarWin_left = from->toolBarWin_left;
    to->toolBarWin_top  = from->toolBarWin_top;
}

static void replace_keeping_geometry(struct PrefsStruct *live, const struct PrefsStruct *with)
{
    struct PrefsStruct geometry = *live;

    *live = *with;
    copy_geometry(live, &geometry);
}

#define TERMINAL_GROUP_FLAGS  (FLAG_PETSCII_MODE | FLAG_USE_XEM_LIBRARY | FLAG_RAW_CONNECTION | FLAG_LOCAL_ECHO)
#define KEYBOARD_GROUP_FLAGS  (FLAG_BS_DEL_SWAP | FLAG_RETURN_CRLF)

static void copy_flags(struct PrefsStruct *to, const struct PrefsStruct *from, ULONG mask)
{
    to->flags = (to->flags & ~mask) | (from->flags & mask);
}

void SitePrefs_ApplyEntry(struct PrefsStruct *live, const struct PrefsStruct *global,
                          const struct SiteSettings *entry)
{
    struct PrefsStruct result = *global;
    const struct PrefsStruct *e = &entry->prefs;

    if (entry->groups & SITE_GROUP_SCREEN)
    {
        result.fontsize      = e->fontsize;
        memcpy(result.fontname, e->fontname, sizeof(result.fontname));
        memcpy(result.color, e->color, sizeof(result.color));
        result.ansiMagic = e->ansiMagic;
        memcpy(result.ansiRGB, e->ansiRGB, sizeof(result.ansiRGB));
    }
    if (entry->groups & SITE_GROUP_TERMINAL)
    {
        memcpy(result.displaydriver, e->displaydriver, sizeof(result.displaydriver));
        memcpy(result.displayidstr, e->displayidstr, sizeof(result.displayidstr));
        copy_flags(&result, e, TERMINAL_GROUP_FLAGS);
    }
    if (entry->groups & SITE_GROUP_KEYBOARD)
        copy_flags(&result, e, KEYBOARD_GROUP_FLAGS);
    if (entry->groups & SITE_GROUP_TRANSFER)
    {
        memcpy(result.xferlibrary, e->xferlibrary, sizeof(result.xferlibrary));
        memcpy(result.xferinit, e->xferinit, sizeof(result.xferinit));
    }
    replace_keeping_geometry(live, &result);
}

void SitePrefs_Restore(struct PrefsStruct *live, const struct PrefsStruct *global)
{
    replace_keeping_geometry(live, global);
}

void SitePrefs_ForSave(struct PrefsStruct *out, const struct PrefsStruct *live,
                       const struct PrefsStruct *global, BOOL sessionActive)
{
    if (sessionActive)
    {
        *out = *global;
        copy_geometry(out, live);
    }
    else
        *out = *live;
}

BOOL SitePrefs_DisplayDiffers(const struct PrefsStruct *a, const struct PrefsStruct *b,
                              BOOL *reopenScreen)
{
    BOOL windows;

    *reopenScreen = a->DisplayID != b->DisplayID
                 || a->DisplayWidth != b->DisplayWidth
                 || a->DisplayHeight != b->DisplayHeight
                 || a->DisplayDepth != b->DisplayDepth
                 || a->fontsize != b->fontsize
                 || strcmp(a->fontname, b->fontname) != 0
                 || ((a->flags ^ b->flags) & SCREEN_FLAGS) != 0;

    windows = ((a->flags ^ b->flags) & WINDOW_FLAGS) != 0
           || strcmp(a->displaydriver, b->displaydriver) != 0
           || memcmp(a->color, b->color, sizeof(a->color)) != 0
           || memcmp(a->ansiRGB, b->ansiRGB, sizeof(a->ansiRGB)) != 0;

    return *reopenScreen || windows;
}

#include <stddef.h>

/* Every field of struct PrefsStruct but flags (carried bit by bit). */
static const struct { size_t offset, size; } prefsFields[] =
{
#define F(m) { offsetof(struct PrefsStruct, m), sizeof(((struct PrefsStruct *)0)->m) }
    F(DisplayID), F(DisplayWidth), F(DisplayHeight), F(DisplayDepth), F(fontsize),
    F(fontname), F(downloadpath), F(xferlibrary), F(xferinit), F(color),
    F(win_left), F(win_top), F(win_width), F(win_height),
    F(sb_left), F(sb_top), F(sb_width), F(sb_height),
    F(uploadpath), F(displaydriver), F(sb_lines), F(displayidstr),
    F(toolBarWin_left), F(toolBarWin_top), F(ansiMagic), F(ansiRGB)
#undef F
};

void SitePrefs_CarryChange(struct PrefsStruct *global, const struct PrefsStruct *before,
                           const struct PrefsStruct *after)
{
    const UBYTE *b = (const UBYTE *)before, *a = (const UBYTE *)after;
    UBYTE *g = (UBYTE *)global;
    ULONG changed = before->flags ^ after->flags;
    size_t i;

    for (i = 0; i < sizeof(prefsFields) / sizeof(prefsFields[0]); i++)
        if (memcmp(b + prefsFields[i].offset, a + prefsFields[i].offset, prefsFields[i].size) != 0)
            memcpy(g + prefsFields[i].offset, a + prefsFields[i].offset, prefsFields[i].size);
    global->flags = (global->flags & ~changed) | (after->flags & changed);
}

void SitePrefs_HandInit(struct SiteHandChanges *h, const struct PrefsStruct *loaded)
{
    h->before = *loaded;
    h->after  = *loaded;
}

void SitePrefs_HandChange(struct SiteHandChanges *h, struct PrefsStruct *global,
                          const struct PrefsStruct *before, const struct PrefsStruct *after,
                          BOOL inSession)
{
    if (inSession)
        SitePrefs_CarryChange(global, before, after);   // the run keeps it
    else
        SitePrefs_CarryChange(&h->before, before, after);   // a real change: saved
    SitePrefs_CarryChange(&h->after, before, after);
}

void SitePrefs_HandForSave(const struct SiteHandChanges *h, struct PrefsStruct *toSave)
{
    SitePrefs_CarryChange(toSave, &h->after, &h->before);
}

BOOL SitePrefs_OnlyLookDiffers(const struct PrefsStruct *a, const struct PrefsStruct *b)
{
    static struct PrefsStruct look;
    BOOL reopenScreen;

    look = *a;
    look.fontsize = b->fontsize;
    memcpy(look.fontname, b->fontname, sizeof(look.fontname));
    memcpy(look.color, b->color, sizeof(look.color));
    look.ansiMagic = b->ansiMagic;
    memcpy(look.ansiRGB, b->ansiRGB, sizeof(look.ansiRGB));
    return !SitePrefs_DisplayDiffers(&look, b, &reopenScreen);
}

static const UBYTE MAGIC_V1[SITE_MAGIC_SIZE] = { 'D', 'C', 'S', '1' };
static const UBYTE MAGIC_V2[SITE_MAGIC_SIZE] = { 'D', 'C', 'S', '2' };
static const UBYTE MAGIC_V3[SITE_MAGIC_SIZE] = { 'D', 'C', 'S', '3' };

/* The 376 bytes every DCTelnet before the ANSI palette block wrote. */
typedef char PrefsV1Layout[offsetof(struct PrefsStruct, ansiMagic) == PREFS_V1_SIZE ? 1 : -1];

size_t SitePrefs_Encode(const struct SiteSettings *s, UBYTE *buf, size_t max)
{
    UBYTE *p = buf;

    if (max < SITE_FILE_SIZE_MAX)
        return 0;
    memcpy(p, MAGIC_V3, SITE_MAGIC_SIZE);            p += SITE_MAGIC_SIZE;
    memcpy(p, &s->groups, sizeof(s->groups));        p += sizeof(s->groups);
    memcpy(p, &s->prefs, sizeof(s->prefs));          p += sizeof(s->prefs);
    memcpy(p, s->fKeys, sizeof(s->fKeys));           p += sizeof(s->fKeys);
    memcpy(p, s->loginMacro, sizeof(s->loginMacro)); p += sizeof(s->loginMacro);
    return (size_t)(p - buf);
}

BOOL SitePrefs_Decode(struct SiteSettings *out, const UBYTE *buf, size_t len,
                      const TEXT *defaultFKeys)
{
    const UBYTE *p = buf + SITE_MAGIC_SIZE;

    size_t prefsSize;

    if (len == SITE_FILE_SIZE_MAX && memcmp(buf, MAGIC_V3, SITE_MAGIC_SIZE) == 0)
        prefsSize = sizeof(out->prefs);
    else if (len == SITE_FILE_SIZE_V2 && memcmp(buf, MAGIC_V2, SITE_MAGIC_SIZE) == 0)
        prefsSize = PREFS_V1_SIZE;
    else if (len == SITE_FILE_SIZE_V1 && memcmp(buf, MAGIC_V1, SITE_MAGIC_SIZE) == 0)
    {
        memset(&out->prefs, 0, sizeof(out->prefs));
        memcpy(&out->prefs, p, PREFS_V1_SIZE);
        Palette_EnsureAnsi(&out->prefs);
        out->groups = SITE_GROUPS_ALL;
        memcpy(out->fKeys, defaultFKeys, sizeof(out->fKeys));
        out->loginMacro[0] = 0;
        return TRUE;
    }
    else
        return FALSE;

    memcpy(&out->groups, p, sizeof(out->groups));        p += sizeof(out->groups);
    memset(&out->prefs, 0, sizeof(out->prefs));
    memcpy(&out->prefs, p, prefsSize);                   p += prefsSize;
    Palette_EnsureAnsi(&out->prefs);
    memcpy(out->fKeys, p, sizeof(out->fKeys));           p += sizeof(out->fKeys);
    memcpy(out->loginMacro, p, sizeof(out->loginMacro));
    out->loginMacro[sizeof(out->loginMacro) - 1] = 0;
    return TRUE;
}

static size_t append_str(char *out, size_t len, size_t max, const char *str)
{
    while (*str && len < max) out[len++] = *str++;
    return len;
}

size_t SitePrefs_NextMacroSegment(const char **cursor, const char *user, const char *pass,
                                  char *out, size_t max, BOOL *wait)
{
    const char *c = *cursor;
    size_t len = 0;

    *wait = FALSE;
    while (*c && len < max)
    {
        if (c[0] == '\\' && c[1])
        {
            switch (c[1])
            {
                case 'u':  len = append_str(out, len, max, user); break;
                case 'p':  len = append_str(out, len, max, pass); break;
                case 'r':  out[len++] = '\r'; break;
                case '\\': out[len++] = '\\'; break;
                case 'd':  *wait = TRUE; break;
                default:   out[len++] = c[0];
                           if (len < max) out[len++] = c[1];
                           break;
            }
            c += 2;
            if (*wait) break;
        }
        else
            out[len++] = *c++;
    }
    *cursor = c;
    return len;
}

BOOL SitePrefs_SwapInKeys(TEXT *live, TEXT *aside, const struct SiteSettings *entry)
{
    if (!(entry->groups & SITE_GROUP_KEYBOARD))
        return FALSE;
    memcpy(aside, live, SITE_FKEY_BYTES);
    memcpy(live, entry->fKeys, SITE_FKEY_BYTES);
    return TRUE;
}

void SitePrefs_SwapOutKeys(TEXT *live, const TEXT *aside)
{
    memcpy(live, aside, SITE_FKEY_BYTES);
}

static BOOL group_differs(ULONG group, const struct PrefsStruct *a, const struct PrefsStruct *b)
{
    /* Apply b's group onto a copy of a: the group differs iff the result
     * is not a. Uses the one definition of the group members above. */
    static struct PrefsStruct result;
    static struct SiteSettings entry;

    entry.groups = group;
    entry.prefs = *b;
    result = *a;
    SitePrefs_ApplyEntry(&result, a, &entry);
    return memcmp(&result, a, sizeof(result)) != 0;
}

ULONG SitePrefs_DifferingGroups(const struct PrefsStruct *a, const struct PrefsStruct *b,
                                const TEXT *keysA, const TEXT *keysB)
{
    ULONG groups = 0;

    if (group_differs(SITE_GROUP_SCREEN, a, b))   groups |= SITE_GROUP_SCREEN;
    if (group_differs(SITE_GROUP_TERMINAL, a, b)) groups |= SITE_GROUP_TERMINAL;
    if (group_differs(SITE_GROUP_TRANSFER, a, b)) groups |= SITE_GROUP_TRANSFER;
    if (group_differs(SITE_GROUP_KEYBOARD, a, b) || memcmp(keysA, keysB, SITE_FKEY_BYTES) != 0)
        groups |= SITE_GROUP_KEYBOARD;
    return groups;
}

/* Bounded string building for the summaries (no printf: this file also runs
 * on the host and in a 68000 build without a C library formatter). */
static void put(char *out, size_t *len, size_t max, const char *str)
{
    while (*str && *len + 1 < max) out[(*len)++] = *str++;
    out[*len] = 0;
}

static void put_num(char *out, size_t *len, size_t max, ULONG n)
{
    char digits[12];
    int i = sizeof(digits) - 1;

    digits[i] = 0;
    do { digits[--i] = (char)('0' + n % 10); n /= 10; } while (n && i > 0);
    put(out, len, max, &digits[i]);
}

static void put_font(char *out, size_t *len, size_t max, const struct PrefsStruct *p)
{
    char name[sizeof(p->fontname)];
    char *dot;

    memcpy(name, p->fontname, sizeof(name));
    name[sizeof(name) - 1] = 0;
    dot = strrchr(name, '.');
    if (dot && strcmp(dot, ".font") == 0) *dot = 0;
    put(out, len, max, name);
    put(out, len, max, " ");
    put_num(out, len, max, p->fontsize);
}

void SitePrefs_GroupSummary(ULONG group, const struct PrefsStruct *p, char *out, size_t max)
{
    size_t len = 0;

    if (max == 0) return;
    out[0] = 0;
    if (group == SITE_GROUP_SCREEN)
    {
        put_font(out, &len, max, p);
    }
    else if (group == SITE_GROUP_TERMINAL)
    {
        if (p->flags & FLAG_PETSCII_MODE)
            put(out, &len, max, "PETSCII Mode");
        else if (p->flags & FLAG_USE_XEM_LIBRARY)
        {
            put(out, &len, max, "XEM ");
            put(out, &len, max, p->displaydriver);
        }
        else
            put(out, &len, max, "ANSI");
        // The terminal type the board is told (TelnetSendTType): PETSCII Mode
        // always says "PETSCII", whatever Display ID holds.
        put(out, &len, max, ", type ");
        put(out, &len, max, (p->flags & FLAG_PETSCII_MODE) ? "PETSCII" : p->displayidstr);
        if (p->flags & FLAG_RAW_CONNECTION) put(out, &len, max, ", Raw Connection");
        if (p->flags & FLAG_LOCAL_ECHO)     put(out, &len, max, ", Local Echo");
    }
    else if (group == SITE_GROUP_KEYBOARD)
    {
        if (p->flags & FLAG_BS_DEL_SWAP) put(out, &len, max, "BS/DEL Swap");
        if (p->flags & FLAG_RETURN_CRLF)
        {
            if (len) put(out, &len, max, ", ");
            put(out, &len, max, "Return = CR + LF");
        }
        if (!len) put(out, &len, max, "Standard keys");
    }
    else if (group == SITE_GROUP_TRANSFER)
    {
        put(out, &len, max, p->xferlibrary[0] ? p->xferlibrary : "No protocol");
        if (p->xferinit[0])
        {
            put(out, &len, max, ", ");
            put(out, &len, max, p->xferinit);
        }
    }
}
