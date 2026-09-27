/* src/site_prefs.h -- per-Address-Book-entry settings (issue #10).
 *
 * `prefs` is the live settings every part of DCTelnet reads. Outside an
 * entry session it is also what DCTelnet.Prefs holds. When the user
 * connects to an Address Book entry that has settings of its own, the live
 * settings become the entry's and the global ones are kept aside; they come
 * back on disconnect, and only they are ever saved. Window geometry is
 * global: where the user put the windows is not part of a board's settings.
 *
 * Pure functions on struct PrefsStruct, unit-tested on the host.
 */
#ifndef SITE_PREFS_H
#define SITE_PREFS_H

#include "prefs.h"

/*
 * An entry overrides settings by GROUP (Term 4.x style): an overridden group's
 * members come from the entry, every other setting from the global ones.
 *   Screen:   DisplayID/Width/Height/Depth, fontname, fontsize, color[16],
 *             FLAG_USE_WORKBENCH
 *   Terminal: FLAG_PETSCII_MODE, FLAG_USE_XEM_LIBRARY, displaydriver,
 *             displayidstr, FLAG_RAW_CONNECTION, FLAG_LOCAL_ECHO (+ XEM options)
 *   Keyboard: FLAG_BS_DEL_SWAP, FLAG_RETURN_CRLF (+ function keys)
 *   Transfer: xferlibrary, xferinit
 * Everything else is app-wide and never comes from an entry.
 */
#define SITE_GROUP_SCREEN    (1UL << 0)
#define SITE_GROUP_TERMINAL  (1UL << 1)
#define SITE_GROUP_KEYBOARD  (1UL << 2)
#define SITE_GROUP_TRANSFER  (1UL << 3)
#define SITE_GROUPS_ALL      (SITE_GROUP_SCREEN | SITE_GROUP_TERMINAL | \
                              SITE_GROUP_KEYBOARD | SITE_GROUP_TRANSFER)

#define SITE_FKEY_BYTES        (10 * 152)   /* F_KEY_COUNT * F_KEY_SIZE (checked in DCTelnet.c) */
#define SITE_LOGIN_MACRO_SIZE  128

/* What one Address Book entry stores besides its BookStruct record. */
struct SiteSettings
{
    ULONG              groups;                         /* SITE_GROUP_* overridden */
    struct PrefsStruct prefs;                          /* values for those groups */
    TEXT               fKeys[SITE_FKEY_BYTES];         /* Keyboard group */
    char               loginMacro[SITE_LOGIN_MACRO_SIZE]; /* entry field, sent after connect */
};

/* live = global with the entry's overridden groups, keeping live's window geometry. */
void SitePrefs_ApplyEntry(struct PrefsStruct *live, const struct PrefsStruct *global,
                          const struct SiteSettings *entry);

/* live = global settings, keeping live's window geometry. */
void SitePrefs_Restore(struct PrefsStruct *live, const struct PrefsStruct *global);

/* What DCTelnet.Prefs must hold: during an entry session the global
 * settings with the live window geometry, otherwise the live settings. */
void SitePrefs_ForSave(struct PrefsStruct *out, const struct PrefsStruct *live,
                       const struct PrefsStruct *global, BOOL sessionActive);

/* TRUE when switching between a and b needs the display reopened;
 * *reopenScreen is set when the screen itself must be reopened too, not
 * only the windows (the same split as MENU_SCREEN_FONT vs MENU_TOOL_BAR). */
BOOL SitePrefs_DisplayDiffers(const struct PrefsStruct *a, const struct PrefsStruct *b,
                              BOOL *reopenScreen);

/* An entry's settings file, PROGDIR:Sites/<id>.prefs:
 *   'DCS2', ULONG groups, PrefsStruct, fKeys, loginMacro       (written)
 *   'DCS1', PrefsStruct -- the first per-entry build's snapshot (read only)
 */
#define SITE_MAGIC_SIZE    4
#define SITE_FILE_SIZE_V1  (SITE_MAGIC_SIZE + sizeof(struct PrefsStruct))
#define SITE_FILE_SIZE_MAX (SITE_MAGIC_SIZE + sizeof(ULONG) + sizeof(struct PrefsStruct) \
                            + SITE_FKEY_BYTES + SITE_LOGIN_MACRO_SIZE)

/* Returns the bytes written (always DCS2), 0 when max is too small. */
size_t SitePrefs_Encode(const struct SiteSettings *s, UBYTE *buf, size_t max);

/* FALSE (out untouched) unless buf is exactly one valid settings file. A DCS1
 * snapshot decodes as every group overridden, fKeys = defaultFKeys, no macro. */
BOOL SitePrefs_Decode(struct SiteSettings *out, const UBYTE *buf, size_t len,
                      const TEXT *defaultFKeys);

#endif /* SITE_PREFS_H */
