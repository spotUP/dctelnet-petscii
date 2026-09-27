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

/* live = entry's settings, keeping live's window geometry. */
void SitePrefs_ApplyEntry(struct PrefsStruct *live, const struct PrefsStruct *entry);

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

#endif /* SITE_PREFS_H */
