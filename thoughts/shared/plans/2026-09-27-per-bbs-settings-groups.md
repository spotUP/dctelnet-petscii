---
date: 2026-09-27
topic: "Per-BBS settings: override groups per Address Book entry (supersedes the #10 full snapshot)"
tags: [dctelnet, address-book, prefs, issue-10]
status: final
---

# Per-BBS settings in override groups -- plan

Research: `thoughts/shared/research/2026-09-27_setting-inventory.md` (35 settings: 18 per-BBS,
12 app-wide, 5 either), `2026-09-27_phonebook-fields-in-other-clients.md` (SyncTERM, Term 4.7,
NComm 3.0), and the 2026-09-25 research for the Address Book and settings internals.

Builds on branch `feature/per-entry-settings` (#10, commits d8140bf..c35450e): global/live
split, sidecar files `PROGDIR:Sites/<id>.prefs`, entry sessions, connect queued behind the
display reopen. This plan changes WHAT an entry holds and how it is edited; that machinery stays.

## Decisions (owner, 2026-09-27)

- G1. Model: Term 4.x style **override groups**. Each entry has an Override switch per group;
  a group that is not overridden follows the global settings (later global changes reach it).
  Replaces #10's D6 full snapshot. Existing `'DCS1'` files load as "all groups overridden".
- G2. Groups and members:
  - **Screen**: screen mode (DisplayID, width, height, depth), font (name, size), palette
    (color[16]), Use Workbench.
  - **Terminal**: PETSCII Mode, Use XEM Library, XEM library name (displaydriver), XEM
    options, Telnet Display ID (displayidstr), Raw Connection, Local Echoback.
  - **Keyboard**: BS/DEL Swap, Return = CR + LF, function keys F1-F10.
  - **Transfer**: transfer protocol library (xferlibrary) and its options string (xferinit).
- G3. App-wide, never per entry: window/scrollback/tool-bar geometry, Disable LEDs, Hide
  TitleBar, Disable Scroll-B, Tool Bar, Packet Window, Jump Scroll, ScrollBack Lines,
  download path, upload path, Address Book sort order.
- G4. Entry fields (always the entry's own, no global counterpart): name, address, port,
  username, password (exist), **comment** (new), **login macro** (new) -- a string sent
  after connecting, `\u` = username, `\p` = password, `\r` = Return (Term/NComm convention).
- G5. **XEM options are saved** -- today they are lost at every display reopen, even
  globally. Globally in `PROGDIR:DCTelnet.XEM`, per entry in `PROGDIR:Sites/<id>.xem`
  (Terminal group), in the XEM library's own format.
- G6. **Function keys per entry** (Keyboard group); global ones stay in `DCTelnet.Keys`.
- G7. UI: the edit window gets Comment and Login Macro fields and a **Settings...** button
  that opens a per-entry settings window: one row per group with an Override checkbox, a
  one-line summary, **Use Current** (copy the group from the settings in use now) and an
  **Edit...** button that opens the existing requester for that group's fields, applied to
  the entry. Settings > Save Settings to Address Book Entry stays as a shortcut: it saves
  the groups that differ from the global settings as overridden.

## Storage

- `BookStruct.res[78]` -> `char comment[64]; char res[14];` (still 256 bytes; the
  compile-time size check guards it). Old versions round-trip the bytes.
- `PROGDIR:Sites/<id>.prefs`, version 2:
  ```
  'DCS2'                 4 bytes magic
  ULONG groupMask        SITE_GROUP_SCREEN | _TERMINAL | _KEYBOARD | _TRANSFER
  struct PrefsStruct     values (only the members of overridden groups are used)
  TEXT fKeys[10*152]     F1-F10 (used when the Keyboard group is overridden)
  char loginMacro[128]   entry field (G4)
  ```
  `'DCS1'` (magic + PrefsStruct) still decodes: mask = all four groups, function keys =
  global, login macro empty. Encode always writes DCS2.
- `PROGDIR:Sites/<id>.xem`: XEM options, written/read by the XEM library (G5).

## Runtime

- `SitePrefs_ApplyEntry(live, global, entry, mask)`: live = global with the members of
  every overridden group copied from the entry; window geometry kept (unchanged rule).
  Pure, host-tested per group.
- Function keys: the global `fKeys` array is kept aside for the session like globalPrefs
  and restored at EndEntrySession.
- XEM options: after the display opens with XEM in use, load `Sites/<id>.xem` when the
  entry overrides Terminal, else `DCTelnet.XEM`; saved when the user changes XEM options.
- Login macro: after a successful Address Book connect, expanded (`\u`, `\p`, `\r`) and
  sent once. The expansion is a pure function, host-tested.

## Phases

1. **Groups in site_prefs** -- group masks, member copy per group, DCS2 encode/decode, DCS1
   upgrade; host tests (per-group apply, geometry kept, DCS1 -> all groups, truncated and
   foreign files rejected, DCS2 round trip). DCTelnet.c/guis.c switch to the new API with
   mask = all groups, so behaviour is unchanged until the UI exists.
2. **Entry fields** -- comment in BookStruct, login macro in the sidecar; edit window gets
   both string gadgets (named tag indexes); login macro expansion + send after connect.
3. **Function keys per entry** -- session swap of fKeys; the Function Keys window can edit
   a given array (entry or global) instead of only the global one.
4. **XEM options saved** -- global and per entry, through the library's own
   `XEmulatorPreferences(xem_io, fileName, XEM_PREFS_LOAD | XEM_PREFS_SAVE)`
   (`src/third_party/Xem/INCLUDE/libraries/xem.h:123-126`, `clib/xem_protos.h:18`): load
   after XEmulatorSetup, save when the user closes the XEM options.
5. **Settings window** -- per-group rows (Override, summary, Use Current, Edit...); the
   existing requesters (screen mode, font, palette, XEM library, transfer protocol,
   function keys) refactored to take the target PrefsStruct instead of writing `prefs`.
   Save Settings to Address Book Entry saves only groups that differ from global.
6. **Verify + land** -- host tests, build, review pass, install + verify on the FS-UAE drive,
   owner test, then one PR for #10 stacked on #14.

## Verification

Automated (`cd test && make`): per-group apply and restore, DCS1/DCS2 files, login macro
expansion (including a password with a backslash), function-key swap as a pure copy.
Reachability exemption (as for #10): the Address Book is Intuition/GadTools on AmigaOS; the
owner's FS-UAE test is the end-to-end proof.

Manual (owner, FS-UAE, `~/Downloads/DCTelnet-PETSCII`):
1. Entry A: Screen overridden (another font), Terminal overridden (PETSCII Mode). Global:
   normal font, PETSCII off. Connect A: C64 screen in the entry's font. Disconnect: global back.
2. Change the global font; entry B with only Keyboard overridden picks up the new font.
3. Entry A's F1 sends its own macro; entry B's F1 the global one.
4. Login macro `\u\r\p\r` logs into the local amiexpress-web board.
5. XEM: choose an XEM library and options for entry C; reconnect -- options kept; restart
   DCTelnet -- global XEM options kept too.
6. A book saved by this build opens in the official 1.9.1: all entries load.
