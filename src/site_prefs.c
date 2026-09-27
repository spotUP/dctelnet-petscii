/* src/site_prefs.c -- per-Address-Book-entry settings (issue #10). */
#include "site_prefs.h"

/* Flags whose change needs the screen reopened (OpenAppScreen reads them). */
#define SCREEN_FLAGS  (FLAG_USE_WORKBENCH | FLAG_HIDE_TITLEBAR | FLAG_PETSCII_MODE)

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

#define SCREEN_GROUP_FLAGS    (FLAG_USE_WORKBENCH)
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
        result.DisplayID     = e->DisplayID;
        result.DisplayWidth  = e->DisplayWidth;
        result.DisplayHeight = e->DisplayHeight;
        result.DisplayDepth  = e->DisplayDepth;
        result.fontsize      = e->fontsize;
        memcpy(result.fontname, e->fontname, sizeof(result.fontname));
        memcpy(result.color, e->color, sizeof(result.color));
        copy_flags(&result, e, SCREEN_GROUP_FLAGS);
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
           || memcmp(a->color, b->color, sizeof(a->color)) != 0;

    return *reopenScreen || windows;
}

static const UBYTE MAGIC_V1[SITE_MAGIC_SIZE] = { 'D', 'C', 'S', '1' };
static const UBYTE MAGIC_V2[SITE_MAGIC_SIZE] = { 'D', 'C', 'S', '2' };

size_t SitePrefs_Encode(const struct SiteSettings *s, UBYTE *buf, size_t max)
{
    UBYTE *p = buf;

    if (max < SITE_FILE_SIZE_MAX)
        return 0;
    memcpy(p, MAGIC_V2, SITE_MAGIC_SIZE);            p += SITE_MAGIC_SIZE;
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

    if (len == SITE_FILE_SIZE_MAX && memcmp(buf, MAGIC_V2, SITE_MAGIC_SIZE) == 0)
    {
        memcpy(&out->groups, p, sizeof(out->groups));        p += sizeof(out->groups);
        memcpy(&out->prefs, p, sizeof(out->prefs));          p += sizeof(out->prefs);
        memcpy(out->fKeys, p, sizeof(out->fKeys));           p += sizeof(out->fKeys);
        memcpy(out->loginMacro, p, sizeof(out->loginMacro));
        out->loginMacro[sizeof(out->loginMacro) - 1] = 0;
        return TRUE;
    }
    if (len == SITE_FILE_SIZE_V1 && memcmp(buf, MAGIC_V1, SITE_MAGIC_SIZE) == 0)
    {
        out->groups = SITE_GROUPS_ALL;
        memcpy(&out->prefs, p, sizeof(out->prefs));
        memcpy(out->fKeys, defaultFKeys, sizeof(out->fKeys));
        out->loginMacro[0] = 0;
        return TRUE;
    }
    return FALSE;
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
