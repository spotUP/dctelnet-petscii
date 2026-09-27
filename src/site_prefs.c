/* src/site_prefs.c -- per-Address-Book-entry settings (issue #10). */
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

void SitePrefs_ApplyEntry(struct PrefsStruct *live, const struct PrefsStruct *entry)
{
    replace_keeping_geometry(live, entry);
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

static const UBYTE MAGIC[SITE_PREFS_MAGIC_SIZE] = { 'D', 'C', 'S', '1' };

size_t SitePrefs_Encode(const struct PrefsStruct *p, UBYTE *buf, size_t max)
{
    if (max < SITE_PREFS_FILE_SIZE)
        return 0;
    memcpy(buf, MAGIC, SITE_PREFS_MAGIC_SIZE);
    memcpy(buf + SITE_PREFS_MAGIC_SIZE, p, sizeof(*p));
    return SITE_PREFS_FILE_SIZE;
}

BOOL SitePrefs_Decode(struct PrefsStruct *out, const UBYTE *buf, size_t len)
{
    if (len != SITE_PREFS_FILE_SIZE || memcmp(buf, MAGIC, SITE_PREFS_MAGIC_SIZE) != 0)
        return FALSE;
    memcpy(out, buf + SITE_PREFS_MAGIC_SIZE, sizeof(*out));
    return TRUE;
}
