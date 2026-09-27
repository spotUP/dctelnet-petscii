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

static struct SiteSettings entry_with(ULONG groups, struct PrefsStruct p) {
    struct SiteSettings e;
    memset(&e, 0, sizeof(e));
    e.groups = groups;
    e.prefs = p;
    return e;
}

/* An overridden group's members come from the entry; everything else --
 * other groups AND the app-wide settings (G3) -- from the global settings;
 * window geometry stays where the windows are. */
static void test_apply_takes_only_overridden_groups(void) {
    struct PrefsStruct global = make(FLAG_BS_DEL_SWAP, "IBM.font", 8), live = global;
    struct PrefsStruct e = make(FLAG_PETSCII_MODE | FLAG_HIDE_LEDS, "Petscii.font", 8);
    struct SiteSettings entry;

    strcpy(e.displayidstr, "PETSCII");
    strcpy(e.xferlibrary, "xprkermit.library");
    e.sb_lines = 9999;                       /* app-wide: never from an entry */
    e.win_left = 99;                         /* geometry: never from an entry */
    live.win_left = 42;

    entry = entry_with(SITE_GROUP_SCREEN | SITE_GROUP_TERMINAL, e);
    SitePrefs_ApplyEntry(&live, &global, &entry);

    assert(strcmp(live.fontname, "Petscii.font") == 0);        /* Screen: entry */
    assert(live.flags & FLAG_PETSCII_MODE);                     /* Terminal: entry */
    assert(strcmp(live.displayidstr, "PETSCII") == 0);
    assert(live.flags & FLAG_BS_DEL_SWAP);                      /* Keyboard: global */
    assert(strcmp(live.xferlibrary, "xprkermit.library") != 0); /* Transfer: global */
    assert(!(live.flags & FLAG_HIDE_LEDS));                     /* app-wide: global */
    assert(live.sb_lines == global.sb_lines);
    assert(live.win_left == 42);
}

static void test_each_group_moves_only_its_members(void) {
    struct PrefsStruct global = make(0, "IBM.font", 8), live;
    struct PrefsStruct e = make(FLAG_RETURN_CRLF | FLAG_USE_WORKBENCH | FLAG_RAW_CONNECTION,
                                "Topaz.font", 11);
    struct SiteSettings entry;

    strcpy(e.xferinit, "TN");
    e.DisplayDepth = 8;

    live = global; entry = entry_with(SITE_GROUP_KEYBOARD, e);
    SitePrefs_ApplyEntry(&live, &global, &entry);
    assert(live.flags == FLAG_RETURN_CRLF && live.DisplayDepth == 4);

    live = global; entry = entry_with(SITE_GROUP_TRANSFER, e);
    SitePrefs_ApplyEntry(&live, &global, &entry);
    assert(strcmp(live.xferinit, "TN") == 0 && live.flags == 0);

    live = global; entry = entry_with(SITE_GROUP_SCREEN, e);
    SitePrefs_ApplyEntry(&live, &global, &entry);
    assert(live.flags == FLAG_USE_WORKBENCH && live.DisplayDepth == 8 && live.fontsize == 11);

    live = global; entry = entry_with(SITE_GROUP_TERMINAL, e);
    SitePrefs_ApplyEntry(&live, &global, &entry);
    assert(live.flags == FLAG_RAW_CONNECTION && live.fontsize == 8);

    live = global; entry = entry_with(0, e);                    /* nothing overridden */
    SitePrefs_ApplyEntry(&live, &global, &entry);
    assert(memcmp(&live, &global, sizeof(live)) == 0);
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

    b = a; b.flags |= FLAG_PETSCII_MODE;                 /* PETSCII fonts: screen */
    assert(SitePrefs_DisplayDiffers(&a, &b, &screen) && screen);

    b = a; b.DisplayDepth = 3;
    assert(SitePrefs_DisplayDiffers(&a, &b, &screen) && screen);

    b = a; b.flags |= FLAG_TOOL_BAR;                     /* windows only */
    assert(SitePrefs_DisplayDiffers(&a, &b, &screen) && !screen);

    b = a; b.color[3] = 0x0F00;                          /* palette: windows */
    assert(SitePrefs_DisplayDiffers(&a, &b, &screen) && !screen);

    b = a; strcpy(b.displaydriver, "xemvt340.library");
    assert(SitePrefs_DisplayDiffers(&a, &b, &screen) && !screen);
}

/* An entry's settings file: 'DCS2', the group mask, the settings, the
 * function keys and the login macro. A truncated, foreign or unknown file
 * is "no settings", never a half-read struct. */
static void test_sidecar_round_trip_and_rejects(void) {
    static struct SiteSettings in, out;
    static TEXT globalKeys[SITE_FKEY_BYTES];
    static UBYTE buf[SITE_FILE_SIZE_MAX + 8];
    size_t n;

    in = entry_with(SITE_GROUP_TERMINAL, make(FLAG_PETSCII_MODE, "Petscii.font", 8));
    strcpy((char *)in.fKeys, "F1 macro");
    strcpy(in.loginMacro, "\\u\\r\\p\\r");
    n = SitePrefs_Encode(&in, buf, sizeof(buf));
    assert(n == SITE_FILE_SIZE_MAX);
    assert(buf[0] == 'D' && buf[1] == 'C' && buf[2] == 'S' && buf[3] == '2');
    memset(&out, 0xAA, sizeof(out));
    assert(SitePrefs_Decode(&out, buf, n, globalKeys));
    assert(memcmp(&in, &out, sizeof(in)) == 0);

    assert(!SitePrefs_Decode(&out, buf, n - 1, globalKeys));   /* truncated */
    buf[3] = '9';
    assert(!SitePrefs_Decode(&out, buf, n, globalKeys));       /* unknown version */
    assert(SitePrefs_Encode(&in, buf, SITE_FILE_SIZE_MAX - 1) == 0);
}

/* Files written by the first per-entry build ('DCS1' + PrefsStruct, a full
 * snapshot) load as every group overridden, with the global function keys
 * and no login macro. */
static void test_a_dcs1_snapshot_loads_as_all_groups(void) {
    static struct SiteSettings out;
    static TEXT globalKeys[SITE_FKEY_BYTES];
    struct PrefsStruct p = make(FLAG_PETSCII_MODE, "Petscii.font", 8);
    UBYTE buf[4 + sizeof(struct PrefsStruct)];

    memcpy(buf, "DCS1", 4);
    memcpy(buf + 4, &p, sizeof(p));
    strcpy((char *)globalKeys, "global F1");
    assert(SitePrefs_Decode(&out, buf, sizeof(buf), globalKeys));
    assert(out.groups == SITE_GROUPS_ALL);
    assert(memcmp(&out.prefs, &p, sizeof(p)) == 0);
    assert(strcmp((char *)out.fKeys, "global F1") == 0);
    assert(out.loginMacro[0] == 0);
    assert(!SitePrefs_Decode(&out, buf, sizeof(buf) - 1, globalKeys));
}

/* Collect the macro's segments; a wait between segments shows as '|'. */
static void expand(const char *macro, const char *user, const char *pass, char *out) {
    const char *cursor = macro;
    char seg[64];
    BOOL wait;
    size_t n;

    out[0] = 0;
    while (*cursor) {
        n = SitePrefs_NextMacroSegment(&cursor, user, pass, seg, sizeof(seg), &wait);
        strncat(out, seg, n);
        if (wait) strcat(out, "|");
    }
}

/* A login macro types the username and password for the user: \u, \p,
 * \r (Return), \d (wait one second), \\ (a backslash). A password with a
 * backslash in it must go out unchanged. */
static void test_login_macro_expands_codes(void) {
    char out[128];

    expand("\\u\\r\\d\\p\\r", "spot", "se\\cret", out);
    assert(strcmp(out, "spot\r|se\\cret\r") == 0);

    expand("x\\\\y\\q", "u", "p", out);          /* \\ -> \, unknown \q kept */
    assert(strcmp(out, "x\\y\\q") == 0);

    expand("", "u", "p", out);
    assert(out[0] == 0);
}

/* An entry that overrides the Keyboard group brings its own F1-F10 for the
 * connection; the global keys are kept aside and come back afterwards. */
static void test_entry_function_keys_swap_in_and_back(void) {
    static TEXT live[SITE_FKEY_BYTES], aside[SITE_FKEY_BYTES];
    static struct SiteSettings entry;

    strcpy((char *)live, "global F1");
    strcpy((char *)entry.fKeys, "entry F1");

    entry.groups = SITE_GROUP_SCREEN;               /* Keyboard not overridden */
    assert(!SitePrefs_SwapInKeys(live, aside, &entry));
    assert(strcmp((char *)live, "global F1") == 0);

    entry.groups = SITE_GROUP_KEYBOARD;
    assert(SitePrefs_SwapInKeys(live, aside, &entry));
    assert(strcmp((char *)live, "entry F1") == 0);
    SitePrefs_SwapOutKeys(live, aside);
    assert(strcmp((char *)live, "global F1") == 0);
}

/* Save Settings to Address Book Entry stores the groups the user changed
 * since connecting -- one field of a group is enough to mark it. */
static void test_differing_groups_names_what_changed(void) {
    static TEXT keysA[SITE_FKEY_BYTES], keysB[SITE_FKEY_BYTES];
    struct PrefsStruct a = make(0, "IBM.font", 8), b = a;

    assert(SitePrefs_DifferingGroups(&a, &b, keysA, keysB) == 0);
    b.flags |= FLAG_HIDE_LEDS; b.win_left = 7; b.sb_lines = 1;     /* app-wide + geometry */
    assert(SitePrefs_DifferingGroups(&a, &b, keysA, keysB) == 0);

    b = a; b.fontsize = 11;
    assert(SitePrefs_DifferingGroups(&a, &b, keysA, keysB) == SITE_GROUP_SCREEN);
    b = a; strcpy(b.displayidstr, "ANSI");
    assert(SitePrefs_DifferingGroups(&a, &b, keysA, keysB) == SITE_GROUP_TERMINAL);
    b = a; strcpy(b.xferinit, "TN");
    assert(SitePrefs_DifferingGroups(&a, &b, keysA, keysB) == SITE_GROUP_TRANSFER);
    b = a; strcpy((char *)keysB, "F1");
    assert(SitePrefs_DifferingGroups(&a, &b, keysA, keysB) == SITE_GROUP_KEYBOARD);
    b = a; b.flags |= FLAG_BS_DEL_SWAP | FLAG_PETSCII_MODE;
    assert(SitePrefs_DifferingGroups(&a, &b, keysA, keysB) == (SITE_GROUP_KEYBOARD | SITE_GROUP_TERMINAL));
}

/* The settings window's one-line summary per group. */
static void test_group_summaries(void) {
    struct PrefsStruct p = make(FLAG_PETSCII_MODE | FLAG_BS_DEL_SWAP, "Petscii.font", 8);
    char out[80];

    strcpy(p.displayidstr, "VT102");    /* PETSCII Mode sends "PETSCII" regardless */
    strcpy(p.xferlibrary, "xprzmodem.library");
    SitePrefs_GroupSummary(SITE_GROUP_SCREEN, &p, out, sizeof(out));
    assert(strcmp(out, "Petscii 8, 640x256, 16 colours") == 0);
    SitePrefs_GroupSummary(SITE_GROUP_TERMINAL, &p, out, sizeof(out));
    assert(strcmp(out, "PETSCII Mode, type PETSCII") == 0);
    SitePrefs_GroupSummary(SITE_GROUP_KEYBOARD, &p, out, sizeof(out));
    assert(strcmp(out, "BS/DEL Swap") == 0);
    SitePrefs_GroupSummary(SITE_GROUP_TRANSFER, &p, out, sizeof(out));
    assert(strcmp(out, "xprzmodem.library") == 0);

    p.flags = FLAG_USE_WORKBENCH | FLAG_USE_XEM_LIBRARY | FLAG_RETURN_CRLF;
    strcpy(p.displaydriver, "xemvt340.library");
    SitePrefs_GroupSummary(SITE_GROUP_SCREEN, &p, out, 20);          /* truncates safely */
    assert(strlen(out) == 19);
    SitePrefs_GroupSummary(SITE_GROUP_TERMINAL, &p, out, sizeof(out));
    assert(strcmp(out, "XEM xemvt340.library, type VT102") == 0);
    SitePrefs_GroupSummary(SITE_GROUP_KEYBOARD, &p, out, sizeof(out));
    assert(strcmp(out, "Return = CR + LF") == 0);
    p.flags = 0;
    SitePrefs_GroupSummary(SITE_GROUP_KEYBOARD, &p, out, sizeof(out));
    assert(strcmp(out, "Standard keys") == 0);
}

int main(void) {
    test_differing_groups_names_what_changed();
    test_group_summaries();
    test_entry_function_keys_swap_in_and_back();
    test_login_macro_expands_codes();
    test_sidecar_round_trip_and_rejects();
    test_apply_takes_only_overridden_groups();
    test_each_group_moves_only_its_members();
    test_a_dcs1_snapshot_loads_as_all_groups();
    test_restore_brings_back_global_but_keeps_moved_windows();
    test_save_writes_global_during_a_session();
    test_display_differs_names_what_needs_a_reopen();
    printf("site_prefs: all assertions passed\n");
    return 0;
}
