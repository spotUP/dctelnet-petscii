---
date: 2026-09-25
topic: "Per-Address-Book-entry settings (upstream issue #10)"
tags: [dctelnet, address-book, prefs, issue-10]
status: final
---

# Per-Address-Book-entry settings -- plan

Research: `thoughts/shared/research/2026-09-25_address-book-internals.md`,
`thoughts/shared/research/2026-09-25_settings-application.md`.

## Decisions already made (owner, 2026-09-25)

- D1. An entry can carry **all** settings (the whole `PrefsStruct`), not a subset.
- D2. Overrides that need a display reopen (font, screen mode, PETSCII, XEM...)
  are applied, and the display reopened, **before** connecting.
- D3. Options-menu changes made while connected to an entry with settings are
  **session only**: on disconnect the global settings return; neither the entry
  nor the global prefs file changes (unless the user saves explicitly, D7).
- D4. Built on `feature/petscii-mode` (PR #14), so PETSCII Mode is per-entry.
- D5. Window geometry (win_*, sb_*, toolBarWin_*) stays global; every other
  `PrefsStruct` field is per entry.
- D6. An entry's settings are a full snapshot, not per-field inheritance.
- D7. Saving: Options > Save Settings to Address Book Entry (while connected
  to an entry) plus Use Current Settings / Use Global Settings buttons in the
  edit window.
- D8. The tab/prefill fix and the delete-row-0 underflow are separate PRs.

## The invariant this feature must hold

`prefs` today is both the live session state and what `SavePrefs()` writes on
quit (and mid-session, Xfer.c:1111). Invariant after this change:

> `DCTelnet.Prefs` on disk only ever holds the **global** settings. Anything
> an entry supplies lives only in memory for the duration of that connection.

Today's code already violates a milder form of this (telnet ECHO negotiation
writes FLAG_LOCAL_ECHO into prefs and it gets saved); that becomes a
session-only change too.

## Design

### Storage: a sidecar file per entry, `DCTelnet.Book` unchanged

`BookStruct` is a 256-byte record with 82 reserved bytes (`res[82]`), no file
header, no version; a changed record size would silently misread every
existing book. `PrefsStruct` is 376 bytes -- it cannot live in the record.

- `res[0..3]` becomes `ULONG settingsId` (0 = entry has no settings of its
  own). Old DCTelnet versions round-trip `res` byte for byte, so a book saved
  by the new version still loads in the old one (entries simply use global
  settings there).
- The entry's settings live in `PROGDIR:Sites/<settingsId>.prefs`: a 4-byte
  magic/version `'DCS1'` followed by a `PrefsStruct` dump (same byte layout
  as `DCTelnet.Prefs`, so the existing load code's size tolerance applies).
- `settingsId` is allocated as max(existing ids)+1 when an entry first gets
  settings; deleting an entry deletes its file. Ids, not names, because names
  are user-editable and not unique.

### Runtime: global vs effective prefs

- New global `struct PrefsStruct globalPrefs;` -- the settings loaded from
  and saved to `DCTelnet.Prefs`.
- `prefs` stays the name every existing reader uses, and becomes the
  **effective** settings. No existing reader changes.
- `SavePrefs()` writes `globalPrefs`, never `prefs`.
- No entry settings active: every menu/requester change is applied to both
  `prefs` and `globalPrefs` (today's behaviour).
- Entry settings active (`sessionSettingsId != 0`): changes go to `prefs`
  only (D3).
- Connect from the Address Book to an entry with settings:
  1. `prefs = entry settings`, then copy the window-geometry fields back
     from `globalPrefs` (D5),
  2. if any display-affecting field differs, `shouldRestart` (+
     `shouldReopenScreen` when screen fields differ) and run the reopen
     BEFORE `BeginServerConnection` (D2),
  3. connect.
- Disconnect (both `DisConnect(remote)` and user disconnect): if
  `sessionSettingsId != 0`, `prefs = globalPrefs`, reopen the display if the
  display-affecting fields differ, `sessionSettingsId = 0`.
- Menus rebuild their check marks from `prefs` on every `OpenDisplay()`
  already, so they show the effective state automatically.

### UI

- Address Book edit window: a cycle/label showing "Settings: Global" or
  "Settings: Own", and two buttons:
  - **Use Current Settings** -- snapshot the current effective `prefs` into
    the entry (creates the sidecar file).
  - **Use Global Settings** -- drop the entry's settings (deletes the file).
- Options menu: **Save Settings to Address Book Entry** (enabled only while
  connected to an entry) -- writes the current effective `prefs` into that
  entry. This is the natural flow: connect, tweak via the normal menus,
  save to the entry.
- The edit window's gadget initial values use hard-coded tag-array indexes
  `[1,8,15,26,33,40]`; adding gadgets requires replacing those with named
  indexes first (small refactor, same commit as the UI).

### Out of scope (separate todos)

- The prefilled-values-do-not-clear-on-tab issue in the same edit window
  (tracked in `thoughts/spot/todos.md`) -- same window, so it may be fixed
  in the same PR if the owner wants.
- `lastcode--` underflow on deleting row 0 (guis.c) -- separate bug fix.

## Verification

Automated (host, `cd test && make`): the global/effective split is plain C
logic -- extract `ApplyEntrySettings()` / `RestoreGlobalSettings()` /
`DisplayFieldsDiffer()` into a small host-testable module (`site_prefs.c`)
with tests: override applied, global restored on disconnect, SavePrefs
source is always global, display-differ detection per field, sidecar file
round-trip and version check, missing/short sidecar falls back to global.

Manual (owner, FS-UAE, `~/Downloads/DCTelnet-PETSCII` drive):
1. Global settings = ANSI/IBM font. Entry A (amiexpress-web) with PETSCII
   Mode + Petscii font. Connect A: display reopens once, C64 screen appears.
2. Disconnect: display returns to the global font, PETSCII Mode unticked.
3. Quit and restart DCTelnet: global prefs unchanged (IBM font, no PETSCII).
4. Old-DCTelnet compatibility: open the saved `DCTelnet.Book` with the
   official 1.9.1 binary -- all entries load, connect with global settings.

## Phases

### Phase 1 -- global/effective split (no UI, no behaviour change)

- `src/site_prefs.h/.c` (new, host-testable, no AmigaOS headers beyond
  exec types via a tiny shim for the host build):
  ```c
  void SitePrefs_ApplyEntry(struct PrefsStruct *effective,
                            const struct PrefsStruct *global,
                            const struct PrefsStruct *entry); /* D5 geometry kept */
  BOOL SitePrefs_DisplayDiffers(const struct PrefsStruct *a,
                                const struct PrefsStruct *b, BOOL *reopenScreen);
  ```
- `DCTelnet.c`: add `globalPrefs`; `LoadPrefs()` fills both; `SavePrefs()`
  writes `globalPrefs`; every menu/requester write goes through one helper
  `SetPref...()`/`CommitPrefs()` that writes `prefs` and, when
  `sessionSettingsId == 0`, `globalPrefs`.
- Success: with no entry settings anywhere, behaviour identical (binary may
  differ); `test/` suite for site_prefs green; FLAG_LOCAL_ECHO from telnet
  ECHO negotiation no longer reaches the saved prefs file.

### Phase 2 -- sidecar storage

- `guis.c`: `BookStruct.res[0..3]` -> `ULONG settingsId` (res shrinks to 78);
  load/save of `PROGDIR:Sites/<id>.prefs` (`'DCS1'` + `PrefsStruct`);
  delete-entry removes the file; id allocation max+1.
- Pure encode/decode in `site_prefs.c` with host tests (round-trip, bad
  magic, short file -> treated as "no settings").
- Success: a book written by the new build loads in the official 1.9.1
  binary (manual check 4); tests green.

### Phase 3 -- apply on connect, restore on disconnect

- Address Book connect path (guis.c:445-455 / 516-531): if the entry has
  settings, `SitePrefs_ApplyEntry`, reopen the display if
  `SitePrefs_DisplayDiffers`, then `BeginServerConnection` (D2).
- `DisConnect()` and user disconnect: restore `prefs = globalPrefs`, reopen
  if needed, `sessionSettingsId = 0`.
- Success: manual checks 1-3.

### Phase 4 -- UI

- Edit window: replace hard-coded tag indexes `[1,8,15,26,33,40]` with named
  constants; add the "Settings: Global/Own" label and the two buttons.
- Options menu: "Save Settings to Address Book Entry" (disabled unless
  `sessionSettingsId != 0` or connected via an entry).
- Success: owner creates entry settings both ways and sees them applied.

### Landing

- Branch `feature/per-entry-settings` from `feature/petscii-mode`; one PR to
  bruno-frederic/dctelnet referencing #10, stacked on #14 (rebased onto
  main once #14 merges). Description saved to `thoughts/shared/prs/`.
