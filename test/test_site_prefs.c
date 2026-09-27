/* test/test_site_prefs.c -- per-Address-Book-entry settings (issue #10). */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "prefs.h"
#include "site_prefs.h"

static struct PrefsStruct make(ULONG flags, const char *font, UWORD size) {
    struct PrefsStruct p;
    memset(&p, 0, sizeof(p));
    p.flags = flags;
    strcpy(p.fontname, font);
    p.fontsize = size;
    p.DisplayID = 0x8000;
    p.DisplayWidth = 640; p.DisplayHeight = 256; p.DisplayDepth = 4;
    p.win_left = 10; p.win_top = 20; p.win_width = 300; p.win_height = 200;
    strcpy(p.displayidstr, "VT102");
    return p;
}

/* The entry's settings replace everything except where the windows are. */
static void test_apply_entry_keeps_window_geometry(void) {
    struct PrefsStruct live = make(0, "IBM.font", 8);
    struct PrefsStruct entry = make(FLAG_PETSCII_MODE, "Petscii.font", 8);
    entry.win_left = 99; entry.sb_lines = 1234;

    SitePrefs_ApplyEntry(&live, &entry);
    assert(live.flags == FLAG_PETSCII_MODE);
    assert(strcmp(live.fontname, "Petscii.font") == 0);
    assert(live.sb_lines == 1234);
    assert(live.win_left == 10); /* geometry stays global */
}

/* Disconnect: the global settings come back, the windows stay where the user
 * moved them during the session. */
static void test_restore_brings_back_global_but_keeps_moved_windows(void) {
    struct PrefsStruct global = make(0, "IBM.font", 8);
    struct PrefsStruct live = make(FLAG_PETSCII_MODE | FLAG_LOCAL_ECHO, "Petscii.font", 8);
    live.win_left = 55;

    SitePrefs_Restore(&live, &global);
    assert(live.flags == 0);
    assert(strcmp(live.fontname, "IBM.font") == 0);
    assert(live.win_left == 55);
}

/* The invariant: DCTelnet.Prefs only ever holds the global settings. During an
 * entry session a menu change or telnet ECHO negotiation lands in the live
 * settings and must not reach the file; outside a session the live settings
 * ARE the global ones (today's behaviour). */
static void test_save_writes_global_during_a_session(void) {
    struct PrefsStruct global = make(0, "IBM.font", 8);
    struct PrefsStruct live = make(FLAG_PETSCII_MODE | FLAG_LOCAL_ECHO, "Petscii.font", 8);
    struct PrefsStruct out;
    live.win_top = 77;

    SitePrefs_ForSave(&out, &live, &global, TRUE);
    assert(out.flags == 0 && strcmp(out.fontname, "IBM.font") == 0);
    assert(out.win_top == 77); /* geometry is global: the moved window is saved */

    SitePrefs_ForSave(&out, &live, &global, FALSE);
    assert(memcmp(&out, &live, sizeof(out)) == 0);
}

static void test_display_differs_names_what_needs_a_reopen(void) {
    struct PrefsStruct a = make(0, "IBM.font", 8), b;
    BOOL screen;

    b = a;
    assert(!SitePrefs_DisplayDiffers(&a, &b, &screen));

    b = a; b.flags |= FLAG_BS_DEL_SWAP;                 /* read live: no reopen */
    assert(!SitePrefs_DisplayDiffers(&a, &b, &screen));

    b = a; b.fontsize = 11;                              /* font: screen reopen */
    assert(SitePrefs_DisplayDiffers(&a, &b, &screen) && screen);

    b = a; b.flags |= FLAG_PETSCII_MODE;   /* C64 display follows the connection, not the flag */
    assert(!SitePrefs_DisplayDiffers(&a, &b, &screen));

    b = a; b.DisplayDepth = 3;
    assert(SitePrefs_DisplayDiffers(&a, &b, &screen) && screen);

    b = a; b.flags |= FLAG_TOOL_BAR;                     /* windows only */
    assert(SitePrefs_DisplayDiffers(&a, &b, &screen) && !screen);

    b = a; b.color[3] = 0x0F00;                          /* palette: windows */
    assert(SitePrefs_DisplayDiffers(&a, &b, &screen) && !screen);

    b = a; strcpy(b.displaydriver, "xemvt340.library");
    assert(SitePrefs_DisplayDiffers(&a, &b, &screen) && !screen);
}

/* The sidecar file an entry's settings live in: magic + the PrefsStruct
 * dump. A file from another version, a truncated one, or garbage is "no
 * settings", never a half-read struct. */
static void test_sidecar_round_trip_and_rejects(void) {
    struct PrefsStruct in = make(FLAG_PETSCII_MODE, "Petscii.font", 8), out;
    UBYTE buf[SITE_PREFS_FILE_SIZE + 8];
    size_t n;

    n = SitePrefs_Encode(&in, buf, sizeof(buf));
    assert(n == SITE_PREFS_FILE_SIZE);
    assert(buf[0] == 'D' && buf[1] == 'C' && buf[2] == 'S' && buf[3] == '1');
    memset(&out, 0xAA, sizeof(out));
    assert(SitePrefs_Decode(&out, buf, n));
    assert(memcmp(&in, &out, sizeof(in)) == 0);

    assert(!SitePrefs_Decode(&out, buf, n - 1));           /* truncated */
    buf[3] = '2';
    assert(!SitePrefs_Decode(&out, buf, n));               /* other version */
    assert(SitePrefs_Encode(&in, buf, SITE_PREFS_FILE_SIZE - 1) == 0); /* no room */
}

int main(void) {
    test_sidecar_round_trip_and_rejects();
    test_apply_entry_keeps_window_geometry();
    test_restore_brings_back_global_but_keeps_moved_windows();
    test_save_writes_global_during_a_session();
    test_display_differs_names_what_needs_a_reopen();
    printf("site_prefs: all assertions passed\n");
    return 0;
}
