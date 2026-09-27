---
date: 2026-09-27
topic: Inventory of every user-changeable DCTelnet setting, classified per-BBS vs app-wide
tags: [research, prefs, settings, address-book, issue-10, site-prefs, petscii]
status: final
---

# DCTelnet setting inventory: per-BBS (P) vs app-wide (A)

Branch `feature/petscii-session-display` @ `508e5df`. Documents what IS. Line
numbers are for this branch (they moved since the 2026-09-25 research on
`feature/petscii-emulation`; that doc's section 2 "when each setting takes
effect" is still correct in substance). `src/DCTelnet.c` has Latin-1 bytes:
all DCTelnet.c line numbers below come from a `tr '\200-\377' '?'` copy.

## What was counted, and from where

| Source | Count | Where |
|---|---|---|
| `struct PrefsStruct` fields | 25 (-> 13 settings below: DisplayID/W/H/Depth = one, win_* x4, sb_* x4, toolBarWin_* x2 grouped) | `src/prefs.h:10-39` |
| `FLAG_*` bits | 16 (bits 0-15) | `src/prefs.h:46-61` |
| Options menu toggles | 16 in `mainMenuDesc`, 13 in the build (3 under `#ifdef _LEGACY_RECEIVE`, not defined in `src/Makefile`/`smakefile`) | `DCTelnet.c:114-134` |
| Settings menu items | 13 = 11 value editors + Snapshot Windows + Save Settings to Address Book Entry (action) | `DCTelnet.c:136-149`, handlers 2581-2689 |
| Login menu | 2 actions (send username / password) | `DCTelnet.c:151-153`, 2691-2699 |
| `struct BookStruct` | 8 fields, 5 user-editable (name, host, port, username, password) | `guis.c:28-38` |
| Files | `PROGDIR:DCTelnet.Prefs`, `.Book`, `.Keys`, `Sites/<id>.prefs`, `WBIcons/`/`SCIcons/`, `Fonts/Petscii*.font` | `DCTelnet.c:258-260`, `guis.c:52`, `guis.c:1545`, `DCTelnet.c:3018-3020` |

Searched and found NOTHING for: `ReadArgs`, `FindToolType`/program tooltypes,
`GetVar`/`SetVar`/`ENV:`/`ENVARC:` (in `src/*.c`, `src/*.h`), charset / codepage
settings. Command line is plain `argv`: `DCTelnet <host> [<port>]`
(`DCTelnet.c:1455-1477`, no ReadArgs template).

Legend, "Effect": **live** = read on every use; **connect** = read at
connect/negotiation; **screen** = display restart with screen reopen;
**windows** = display restart, windows only; **xfer** = read when a transfer
starts; **open** = read when that window opens.
"Snap" = captured today by the per-entry snapshot `PROGDIR:Sites/<id>.prefs`
(whole PrefsStruct minus the 10 geometry fields, `site_prefs.c:14-26`).

## 1. PrefsStruct value settings (not flags)

| # | Setting (menu) | Storage | Effect | Snap | Class | Reason from the code path |
|---|---|---|---|---|---|---|
| 1 | Screen mode (Settings > Screen Mode.., `ChooseScreen` 759) | `DisplayID`, `DisplayWidth/Height/Depth` | screen, but the menu restarts only if ID or depth changed (2582-2590); a width/height-only change waits for Reset Screen. Disabled on WB (3091) | yes | ? | Sets the cell grid: an 80x25 ANSI board and a 40x25 PETSCII board want different sizes, but the mode is also what the user's monitor can show |
| 2 | Screen font (Settings > Screen Font..) | `fontname[32]`, `fontsize` | screen (2593-2601, `OpenAppScreen` 3001) | yes | P | The font is the glyph set: IBM/CP437 art boards vs topaz/Amiga boards render differently; PETSCII already swaps to its own font per session |
| 3 | Screen palette (Settings > Screen Palette.., `ChoosePalette` 817) | `color[16]` RGB4 | live on the viewport; `LoadRGB4` at window open (3130). Disabled on WB (3092) | yes | ? | Amiga-ANSI boards and PC-ANSI boards assume different colour sets, but the palette is also user taste |
| 4 | Download path (Settings > Download Path..) | `downloadpath[52]` | xfer (`Xfer.c:1162`, shown 555) | yes | ? | Only where received files land on this machine; per-board download dirs are a common terminal feature, nothing in the protocol depends on it |
| 5 | Upload path (no menu: set by the upload / ASCII Send file requester) | `uploadpath[52]` | xfer (`Xfer.c:391,1089`); written + `SavePrefs()` at `Xfer.c:1110-1111` | yes | A | A "last directory used" memory, not a choice about the board |
| 6 | Transfer protocol (Settings > Transfer Protocol..) | `xferlibrary[52]` (default `xprzmodem.library`, 1392) | xfer (`ProtoStart` `Xfer.c:876`; auto-Zmodem 1106-1107) | yes | P | The board decides which protocols it offers |
| 7 | Protocol options (Settings > Protocol Options..) | `xferinit[52]` (default `TC,OR,B32,FO,AN,DN,KY,SN,RN`, 1393) | xfer (`Xfer.c:902` passed as `xpr_filename` to `XProtocolSetup`) | yes | P | Block size / resume / overwrite behaviour tuned to the board's transfer side |
| 8 | XEM library (Settings > XEM Library..) | `displaydriver[32]` | windows if XEM is on (2632-2641); opened at `Xem_wrapper.c:118` | yes | P | Selects the terminal emulator (VT, Amiga, etc.) the board expects |
| 9 | Telnet Display ID (Settings > Telnet Display ID..) | `displayidstr[32]` (default `VT102`, 1415) | connect: sent only on server `SB TTYPE SEND` (`DCTelnet-protocol.h:348-371`); replaced by `PETSCII` in PETSCII mode (356) | yes | P | The TTYPE string is how a board picks ANSI/VT/PETSCII output |
| 10 | ScrollBack lines (Settings > ScrollBack Lines..) | `sb_lines` (0 -> 300, 1413) | live (`AddBuf` 899) | yes | A | A memory budget of the app, nothing to do with the board |
| 11 | Main window geometry (Settings > Snapshot Windows) | `win_left/top/width/height` | open | no | A | Where the user put the window; `site_prefs.c` already keeps it global |
| 12 | Scroll-back window geometry (Snapshot Windows) | `sb_left/top/width/height` | open (`guis.c` scroll-back open) | no | A | Same |
| 13 | Tool bar position (Snapshot Windows) | `toolBarWin_left/top` | open (`guis.c` tool bar open) | no | A | Same |

## 2. FLAG_* bits (Options menu)

| # | Setting (Options >) | Bit | Effect | Snap | Class | Reason from the code path |
|---|---|---|---|---|---|---|
| 14 | Use Workbench (W) | 3 `FLAG_USE_WORKBENCH` | screen (2471-2475, 2998) | yes | ? | UI placement, BUT it also picks the console device: `console.device` on WB vs `ibmcon.device` on the own screen (3378), i.e. whether IBM/ANSI-BBS emulation is available; also forces LEDs off / packet window off |
| 15 | Disable LEDs (I) | 2 `FLAG_HIDE_LEDS` | live (2477-2489, `LEDs()`) | yes | A | Title-bar activity lamps, UI chrome |
| 16 | Hide TitleBar (R) | 0 `FLAG_HIDE_TITLEBAR` | screen (2491-2495, `SA_ShowTitle`) | yes | A | UI chrome |
| 17 | CRLF Correction (L) - no menu item in this build | 1 `FLAG_CRLF_CORRECTION` | none: only menu glue reads it (2498, 3240) | yes | P (dead) | Line-ending repair is a per-board problem (issue #10 asks for it per entry), but the bit does nothing today |
| 18 | BS/DEL Swap (/) | 4 `FLAG_BS_DEL_SWAP` | live (`OutKey` 2058, key path 2316; skipped in PETSCII) | yes | P | Which erase code the board's line editor accepts |
| 19 | Disable Scroll-B (E) | 5 `FLAG_DISABLE_SCROLLBACK` | live (`Receive` 1003, 1075, 1083) | yes | A | Whether the app keeps a history; speed/memory choice |
| 20 | Strip Colour (J) - no menu item in this build | 6 `FLAG_STRIP_COLOUR` | none (2510 handler unreachable, 3245) | yes | P (dead) | Would be per board (colour vs mono boards); no reader |
| 21 | Simple Telnet (1) - no menu item in this build | 7 `FLAG_SIMPLE_TELNET` | none (2517, 3246) | yes | P (dead) | Would be per server negotiation quirk; no reader |
| 22 | Packet Window (2) | 8 `FLAG_PACKET_WINDOW` | windows (2520-2526, `OpenAppWindow`); impossible on WB | yes | ? | A line-editing input bar: useful for line-mode boards/MUDs, otherwise a UI preference |
| 23 | Use XEM Library (3) | 9 `FLAG_USE_XEM_LIBRARY` | windows (2528-2540, `CreateAppMenus` 3222-3231, XEM init 3346-3354); cleared if no library or init fails | yes | P | Built-in ANSI vs external emulator is a per-board emulation choice |
| 24 | Tool Bar (4) | 10 `FLAG_TOOL_BAR` | windows; live on WB (2542-2552); cleared when no icons (`guis.c:1575`) | yes | A | UI chrome |
| 25 | Return = CR + LF (5) | 11 `FLAG_RETURN_CRLF` | live (key path 2323, packet window 2117) | yes | P | Which line ending the board expects from Return |
| 26 | Local Echoback (6) | 12 `FLAG_LOCAL_ECHO` | live (`OutKey` 2085); ALSO written by the server's telnet ECHO negotiation (`SetLocalEchoBack` 2021-2050, protocol.h 577/673) | yes | P | Echo is negotiated per server; raw/non-negotiating boards need it set by hand |
| 27 | Raw Connection (7) | 13 `FLAG_RAW_CONNECTION` | live (`Receive` 1000, `OutKey` IAC doubling 2073, Zmodem 1103, `Xfer.c:71,101,225`); `Finger` sets it temporarily (1302-1324) | yes | P | Telnet vs raw TCP is a property of the server |
| 28 | Jump Scroll (8) | 14 `FLAG_JUMP_SCROLL` | windows (console unit 2 vs CHARMAP, 3399); disabled with XEM | yes | A | Rendering speed of the console, no board involvement |
| 29 | PETSCII Mode (9) | 15 `FLAG_PETSCII_MODE` | live + connect + screen: `PETSCII_SESSION()` (189) gates translator, keys, fonts; `SyncPetsciiDisplay` 602 | yes | P | C64 boards vs everything else |

## 3. Settings NOT in PrefsStruct

| # | Setting (menu) | Storage | Effect | Snap | Class | Reason from the code path |
|---|---|---|---|---|---|---|
| 30 | Function keys F1-F10 (Settings > Function Keys.., `guis.c:1262-1480`) | `fKeys[10*152]` (`DCTelnet.h:116-118`), raw file `PROGDIR:DCTelnet.Keys` (load 1418-1422, save `guis.c:1467-1470`) | live: CSI digit -> `SendMacro` (2261-2265; `\n \r \e \NNN` escapes 1124-); PETSCII sessions send C64 F-keys instead (2272) | **no** | P | Macros are board commands and logins; the snapshot does not include them |
| 31 | XEM Lib Options (Settings > XEM Lib Options.., only when XEM is active) | inside the XEM library's `xemIO` only; `XEmulatorPreferences` is never called (only in `third_party/Xem`), comment at 3224 "state not saved in prefs" | live until the next display restart: `XEmulatorCleanup` in `UninitializeXemLibrary` (`Xem_wrapper.c:203`) discards them | **no, and not even saved globally** | P | Emulator options (e.g. VT modes) are what the board expects |
| 32 | Entry: Site Name, Address, Port (Address Book > Edit) | `BookStruct.name/host/port`, `PROGDIR:DCTelnet.Book` (256-byte records) | connect | n/a (already per entry) | P | They are the board |
| 33 | Entry: Username, Password (Address Book > Edit) | `BookStruct.username/password` -> globals `username/password` at connect (`DCTelnet.c` `RunPendingConnect`) | live, only via Login > Send Username / Send Password (2691-2699); no auto-login | n/a (per entry) | P | Credentials per board |
| 34 | Entry: Settings (Address Book > Edit: Use Current / Use Global; Settings > Save Settings to Address Book Entry) | `BookStruct.settingsId` -> `PROGDIR:Sites/<id>.prefs` ('DCS1' + PrefsStruct) | connect: `BeginEntrySession` 635 applies, `EndEntrySession` 660 restores | - | (mechanism, not counted) | The per-entry container itself |
| 35 | Address Book sort order (Address Book > sort cycle, `GD_SORT`) | none: reset to mode 0 on every open (`guis.c:568-569`) | live in the window | no | A | Presentation of the list |
| 36 | Tool bar icons and their hover text | `PROGDIR:WBIcons/` or `PROGDIR:SCIcons/<name>.info`; `ToolTypes[0]` = hover text (`guis.c:1545-1563, 1627-1630`); button actions are fixed (2141-2154) | open | no | A | Look of the app |

## 4. Inputs that are not persisted settings (not counted)

| Item | Where | Note |
|---|---|---|
| Command line `DCTelnet <host> [<port>]` | `main` 1455-1477 | one-shot connect target; no ReadArgs, no tooltypes |
| Connect requester `host:port` | `OnConnectClicked` 1868-1925 | prefilled from the global `server`, not saved; "Connect (New instance)" runs `run <prog> <host> <port>` (1909-1917) |
| XPR library's own options dialog | `XferOptions()` `Xfer.c:910-939` | unreachable: Protocol Options.. only edits the string (TODO at 2626). DCTelnet never reads `ENV:xprzmodem` (the XPR convention, `third_party/Xpr/XprZmodem.Doc:133-137`); it always passes `prefs.xferinit` |
| PETSCII fonts | `FONTS:` or `PROGDIR:Fonts/Petscii.font`, `PetsciiLower.font` (3018-3020) | fixed names, a resource not a setting |
| Hard-wired behaviour with no switch | Zmodem auto-start on `**\x18B` (1007-1107); telnet always requests BINARY + SGA (protocol.h 607-614); NAWS sent from the window/XEM size after `DO NAWS` (394-445, never re-sent after a display restart); PETSCII grid fixed 40x25 (`petscii_dispatch_init`) | candidates for per-board switches, not settings today |
| Debug capture | `PROGDIR:capture_in.bin` (991) | `_DEBUG` builds only |

## 5. Tally

35 settings rows classified (rows 1-33, 35, 36; row 34 is the mechanism):

| Class | Count | Rows |
|---|---|---|
| P | 18 | 2, 6, 7, 8, 9, 17, 18, 20, 21, 23, 25, 26, 27, 29, 30, 31, 32, 33 (17, 20, 21 are dead bits) |
| A | 12 | 5, 10, 11, 12, 13, 15, 16, 19, 24, 28, 35, 36 |
| ? | 5 | 1 (screen mode), 3 (palette), 4 (download path), 14 (Use Workbench), 22 (Packet Window) |

## 6. Facts that bear on the per-entry design

- The snapshot today carries every non-geometry PrefsStruct field, so the A
  settings 5, 10, 15, 16, 19, 24, 28 (upload path, scrollback lines, LEDs,
  title bar, scrollback off, tool bar, jump scroll) are per entry too once an
  entry has settings. Only geometry is excluded (`site_prefs.c:14-26`).
- The two P settings outside PrefsStruct are NOT covered: function keys (30,
  one global `DCTelnet.Keys`) and XEM options (31, never persisted anywhere).
- `FLAG_LOCAL_ECHO` is rewritten by server negotiation during a session; with
  an entry session it lands in the live copy, and `SavePrefs` writes only
  `globalPrefs` (`DCTelnet.c:579`), so it no longer leaks into the global file.
- A mid-session `SavePrefs()` from an upload (`Xfer.c:1111`) writes the global
  settings plus live geometry, so the new `uploadpath` chosen in an entry
  session is not saved globally (it lives only in the entry's live copy until
  "Save Settings to Address Book Entry").
- Screen-mode W/H-only changes do not trigger a restart from the menu, but
  `SitePrefs_DisplayDiffers` (`site_prefs.c:56-73`) compares W and H, so an
  entry switch does reopen for them.

## Could not confirm

- Whether a given `xpr*.library` binary reads `ENV:<libname>` by itself when
  `xpr_filename` is set: the bundled xprzmodem source (`third_party/Xpr/Zm.c`,
  `Utils.c`) has no `GetVar`/`ENV` reference; other XPR libraries not checked.
- What options a given XEM library exposes (row 31): library-specific, not in
  this repo.
- Upstream issue #10's exact list was taken from the 2026-09-25 address-book
  research, not re-read from GitHub.
