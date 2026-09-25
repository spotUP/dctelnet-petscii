---
date: 2026-09-25
topic: How DCTelnet global settings (PrefsStruct) are loaded, saved and applied
tags: [research, prefs, settings, address-book, issue-10, petscii, display-restart]
status: final
---

# DCTelnet settings: load, save, apply

Branch `feature/petscii-emulation` @ `8d2afb7`. Input for upstream issue #10
(per-Address-Book-entry configuration). Documents what IS.

Note: `src/DCTelnet.c` and `src/DCTelnet-protocol.h` hold Latin-1 bytes; use
`grep -a` or `sed -n`.

## 1. PrefsStruct, flags, file

`struct PrefsStruct` — `src/DCTelnet.h:8-35`; single global `prefs` —
`src/DCTelnet.c:202` (BSS, zero-initialised).

| Field | Meaning | Set by |
|---|---|---|
| `DisplayID`, `DisplayWidth/Height/Depth` | custom-screen mode | Settings > Screen Mode.. (`ChooseScreen`, DCTelnet.c:592-648) |
| `fontsize`, `fontname[32]` | screen/console font | Settings > Screen Font.. (DCTelnet.c:2405-2413) |
| `downloadpath[52]` | XPR download dir | Settings > Download Path.. (2420); read Xfer.c:555,1162 |
| `xferlibrary[52]` | XPR library | Settings > Transfer Protocol.. (2425); read at Upload/Download |
| `xferinit[52]` | XPR options string | Settings > Protocol Options.. (2433); read Xfer.c:902 |
| `color[16]` | RGB4 palette | Settings > Screen Palette.. (`ChoosePalette` 650-672) |
| `flags` | FLAG_* bits (below) | Options menu, plus code writes (see §5) |
| `win_left/top/width/height` | WB-mode main window geometry | Settings > Snapshot Windows (2476-2493) |
| `sb_left/top/width/height` | scrollback window geometry | Snapshot Windows; read guis.c:850-953 |
| `uploadpath[52]` | last upload dir | ASCII Send (2232), Xfer.c:1110 |
| `displaydriver[32]` | XEM library name | Settings > XEM Library.. (2443-2452); read Xem_wrapper.c:118 |
| `sb_lines` | scrollback line limit | Settings > ScrollBack Lines.. (2471-2474); read AddBuf 732 |
| `displayidstr[32]` | telnet TTYPE string | Settings > Telnet Display ID.. (2464-2469) |
| `toolBarWin_left/top` | tool bar window position | Snapshot Windows; read guis.c:1299-1300 |

FLAG_* bits (`src/DCTelnet.h:42-57`), menu items (`mainMenuDesc`, DCTelnet.c:113-133):

| Bit | Flag | Options menu item |
|---|---|---|
| 0 | FLAG_HIDE_TITLEBAR | Hide TitleBar (R) |
| 1 | FLAG_CRLF_CORRECTION | CRLF Correction (L) — only `#ifdef _LEGACY_RECEIVE` (117-119) |
| 2 | FLAG_HIDE_LEDS | Disable LEDs (I) |
| 3 | FLAG_USE_WORKBENCH | Use Workbench (W) |
| 4 | FLAG_BS_DEL_SWAP | BS/DEL Swap (/) |
| 5 | FLAG_DISABLE_SCROLLBACK | Disable Scroll-B (E) |
| 6 | FLAG_STRIP_COLOUR | Strip Colour (J) — legacy only (122-125) |
| 7 | FLAG_SIMPLE_TELNET | Simple Telnet (1) — legacy only |
| 8 | FLAG_PACKET_WINDOW | Packet Window (2) |
| 9 | FLAG_USE_XEM_LIBRARY | Use XEM Library (3) |
| 10 | FLAG_TOOL_BAR | Tool Bar (4) |
| 11 | FLAG_RETURN_CRLF | Return = CR + LF (5) |
| 12 | FLAG_LOCAL_ECHO | Local Echoback (6) |
| 13 | FLAG_RAW_CONNECTION | Raw Connection (7) |
| 14 | FLAG_JUMP_SCROLL | Jump Scroll (8) |
| 15 | FLAG_PETSCII_MODE | PETSCII Mode (9) — shortcut "9" is also used by Telnet Display ID.. (145) |

`_LEGACY_RECEIVE` is not defined by `src/Makefile`/`smakefile`, so bits 1, 6, 7
have no menu item and **no reader anywhere** in the current build (only
`UpdatePrefsFlagFromMenu`/`SetNewMenuCheckFromPref` touch them: 2310, 2322,
2329, 3045, 3050-3051). They are dead persisted bits.

**File.** `prefsFilename = "PROGDIR:DCTelnet.Prefs"` (DCTelnet.c:245). Format:
raw binary dump of the struct, `sizeof(struct PrefsStruct)` = 376 bytes
(68k 2-byte alignment, no padding; padding warnings removed in `895ba12`).
Related files: `PROGDIR:DCTelnet.Book` (246), `PROGDIR:DCTelnet.Keys` (247).

**SavePrefs** (528-536): `Open(MODE_NEWFILE)` + `Write(&prefs, sizeof)`. No
versioning, no checksum.

**LoadPrefs** (1164-1252), called once from `main` (1369):
- File read; if `< 252` bytes ("old config") -> `goto fixprefs` (1171-1184),
  which resets window/scrollback geometry and saves. A read of 252..375 bytes is
  accepted as-is; later fields stay zero (BSS) and get defaults below.
- No file, or `DisplayID == DEFAULT_MONITOR_ID` (set when OpenDisplay failed last
  run, 3133) -> first-time path: info requester + `ChooseScreen(TRUE)` (defaults
  PAL HIRES 640x256x4, 598-601).
- First-time defaults (1214-1230): `fontsize=8`, `fontname="topaz.font"`,
  `xferlibrary="xprzmodem.library"`, `xferinit="TC,OR,B32,FO,AN,DN,KY,SN,RN"`,
  `color` = static `color[]` (241-243), `flags = FLAG_TOOL_BAR`, `win_top=11`,
  `win_width=640`, `win_height=200`, `sb_top=12`, `sb_width=640`,
  `sb_height=DisplayHeight/2-4`, then `SavePrefs()`.
- Always: `sb_lines==0 -> 300` (1238); empty `displayidstr -> "VT102"` (1240).
- Also loads `fKeys` from `DCTelnet.Keys` (1243-1249).

## 2. When each setting takes effect

Legend: **(a)** read live on every use; **(b)** read at connect / negotiation
time; **(c)** needs display restart (`shouldRestart`, with or without
`shouldReopenScreen`); **(w)** read when a window is opened.

| Setting | Class | Code path |
|---|---|---|
| FLAG_BS_DEL_SWAP | a | `OutKey` 1870-1877 (skipped in PETSCII, 2126-2130) |
| FLAG_RETURN_CRLF | a | key path 2135; packet window 1929 |
| FLAG_LOCAL_ECHO | a | `OutKey` 1897; also WRITTEN by telnet ECHO negotiation (protocol.h 584/588/677 -> `SetLocalEchoBack` 1833-1866) |
| FLAG_RAW_CONNECTION | a (+b) | `Receive` 833 selects raw vs telnet parser per recv; `OutKey` IAC doubling 1885; Zmodem 936. Telnet ctx is reset at connect (2752) regardless |
| FLAG_DISABLE_SCROLLBACK | a | `Receive` 836, 908, 916 |
| FLAG_HIDE_LEDS | a | `LEDs()` 1016; main loop 1553/1559; connect 2659; menu handler repaints (2289-2300) |
| FLAG_PETSCII_MODE | a + b + c | a: `Receive` 878 (translator), key path 2073-2131. b: TTYPE string "PETSCII" (protocol.h 356), `petscii_dispatch_init` + font warning at connect (2763-2766). c: fonts opened in `OpenAppScreen` 2817-2826, `SetFont` 3014, `PETSCII_CONSOLE_SETUP` ("\x1b[?7l") in `OpenDisplay` 3249-3250. Menu: restart + reopen screen (2383-2392) |
| FLAG_USE_WORKBENCH | c (screen) | `OpenAppScreen` 2804-2805; menu 2283-2287 |
| FLAG_HIDE_TITLEBAR | c (screen) | `SA_ShowTitle` 2853, window layout 2939, toolbar guis.c:1321; menu 2303-2307 |
| FLAG_PACKET_WINDOW | c (windows only) | `OpenAppWindow` 2971; menu 2332-2338 (forced off on WB, 2831) |
| FLAG_USE_XEM_LIBRARY | c (windows only) | `CreateAppMenus` 3028 sets `drivertype`; `OpenDisplay` 3149-3169 inits XEM; menu 2340-2352 |
| FLAG_TOOL_BAR | c (windows only), live on WB | `OpenAppWindow` 2925/2937; menu 2354-2364 |
| FLAG_JUMP_SCROLL | c (windows only) | console unit number in `OpenDisplay` 3204-3207; menu 2377-2381 |
| FLAG_CRLF_CORRECTION, _STRIP_COLOUR, _SIMPLE_TELNET | none | no reader (see §1) |
| DisplayID/Width/Height/Depth | c (screen) | `OpenAppScreen` 2838-2857; menu restarts only if ID or depth changed (2394-2403) |
| fontname, fontsize | c (screen) | `OpenAppScreen` 2807-2815 (topaz 8 fallback); `fontsize` also read live for LED/layout geometry |
| color[16] | w (full-screen) | `LoadRGB4` in `OpenAppWindow` 2935; palette requester changes the viewport live |
| displaydriver | c (windows only) | `InitializeXemLibrary` -> Xem_wrapper.c:118; menu restarts if XEM on (2452) |
| displayidstr | b | `TelnetSendTType` protocol.h 348-371, sent only on server `SB TTYPE SEND` (466-486) — normally once, early in the session |
| (NAWS size) | b | `TelnetSendWindowSize` protocol.h 394-445, called ONLY after `DO NAWS` (767-771); derived from window/font or XEM, not prefs directly. Never re-sent after a display restart |
| sb_lines | a | `AddBuf` 732 |
| downloadpath, uploadpath, xferlibrary, xferinit | a (at transfer) | Xfer.c, menu 2218-2248, Receive 939-940 |
| win_*, sb_*, toolBarWin_* | w | `OpenAppWindow` 2899-2902, guis.c:934-953, guis.c:1299 |

## 3. Display restart path

Trigger: menu handlers set `shouldRestart` (and optionally `shouldReopenScreen`);
the main loop, after processing window messages, runs (1589-1597):

```c
CloseDisplay(shouldReopenScreen);
if (! OpenDisplay()) goto clean_exit;
```

`MENU_RESET_SCREEN` (2208-2211) forces a full restart with screen reopen.

**CloseDisplay(manageScreen)** (3331-3448) always: `UninitializeXemLibrary()`;
`CloseDevice` of the console (ibmcon.device / console.device) with the sigbit-31
repair (3355-3373, `AllocSignal(31)`); deletes IORequest + MsgPort; closes
`packetWin`, `win` (and its menu strip), scrollback window (`CloseScrollBack`),
tool bar (`CloseToolBarWindow`); `FreeMenus`. If `manageScreen`: frees
VisualInfo/DrawInfo, `CloseScreen` (unless on WB), closes `ansiFont`,
`petsciiFont`, `petsciiFontLower`. Sets `isAppIconified = TRUE` (3441), which
makes `ConWrite` (257) drop output until `OpenDisplay` clears it (3246).

**OpenDisplay()** (3118-3309): `OpenAppScreen()` only if `scr == NULL`
(3127-3128); VisualInfo/DrawInfo; `OpenAppWindow()`; `CreateAppMenus()`
(re-derives menu checks from `prefs.flags`); XEM init (falls back and clears
FLAG_USE_XEM_LIBRARY on failure, 3149-3169); console device open (unit from
FLAG_JUMP_SCROLL); PETSCII console setup; `LEDs()`; prints the startup banner
only `if(!isConnected)` (3254). On screen-open failure: sets
`DisplayID = DEFAULT_MONITOR_ID` and `SavePrefs()` (3130-3136).

**Does TCP survive?** Yes by design: neither function touches `tcpSocket`,
`isConnected`, `telnetCtx` or `zmodemCtx`; the loop picks up the socket on the
next `WaitSelect`. Upstream issue #3 ("connection stuck/freeze after display
changes") was caused by ibmcon.device freeing signal bit 31 on `CloseDevice`;
workaround commit `b24b33b` ("fix: AllocSignal 31 as workaround for issue #3")
allocates sig 31 at startup (`dontUseSig31`, 1340) and re-allocates it after `CloseDevice` (3355-3373); freed at exit 1626.

**State lost / kept on restart:**
- Lost: visible console contents and console-device state (cursor, attributes,
  charmap) — a fresh window + device; XEM emulator state; open scrollback/packet
  windows (scrollback window is not reopened).
- Kept: `scrollbackList` (freed only at exit, `ClearScrollBack` 1098/1611);
  `g_petsciiState` (not re-inited by restart, only by the PETSCII toggle and at
  connect); telnet negotiation state; `server`, `tcpPort`, `username/password`.
- Not re-sent: NAWS (window size may change with font/mode), TTYPE.

## 4. Connect and disconnect

Entry points: `OnConnectClicked` (1683-1738, host:port requester), Address
Book (guis.c:524-529: `tcpPort = conbook->port; BeginServerConnection(host, port)`,
then copies `username`/`password`), command line (`main` 1414-1415), `Finger`
(1118-1152).

`BeginServerConnection` (2609-2630) spawns the "Connecting..." task and calls
`EstablishTCPConnection` (2643-2775), which:
- reads `prefs.flags & (HIDE_TITLEBAR|HIDE_LEDS)` for the activity LED (2658-2662);
- calls `DisConnect(FALSE, FALSE)` first (2665) — any previous session ends;
- copies host into global `server` (2672-2673); resolves; connects;
- `ResetTelnetContext()` + `ResetZmodemContext()` (2751-2752; legacy variant
  reads FLAG_RAW_CONNECTION, 2743-2749);
- `petscii_dispatch_init(&g_petsciiState, 40, 25)` (2763) and warns if
  PETSCII Mode is on but `petsciiFont` is NULL (2764-2766);
- sets `isConnected = TRUE`, `LEDs()`.
It does not read any other prefs; everything else is read live afterwards.
Telnet client-side negotiation (`TelnetNegotiateRequiredOptions`, protocol.h
602-615) is triggered from `Receive` (923-928) once server negotiation is seen.

`DisConnect(remote, quiet)` (486-526): prints elapsed time, `shutdown` +
`CloseSocket`, `isConnected = FALSE`, `ResetTelnetContext()` +
`ResetZmodemContext()`, zeroes byte counters, re-enables the Options menu after
a Finger request, `LEDs()`. It restores no prefs. Called from menu Disconnect
(2272), connect (2665), `Receive` on EOF/error (816), exit (1608).

Per-connection reset hooks that exist today: `ResetTelnetContext`,
`ResetZmodemContext`, `petscii_dispatch_init` (connect only). There is no
"connection ended" hook beyond `DisConnect` itself.

Precedent for a temporary override: `Finger` sets FLAG_RAW_CONNECTION around
`BeginServerConnection` and restores only that bit afterwards (1131-1149;
commit `131d1f5` "Finger() function does not reset all Options flags anymore").
The flag is restored immediately after connect, not at disconnect.

Address Book entry: `struct BookStruct` (guis.c:27-36) = `name[32]`, `host[52]`,
`port`, `lastConnect`, `username[42]`, `password[42]`, `res[82]` (82 reserved
bytes); stored as raw struct array in `DCTelnet.Book` (read guis.c:344-366,
write 542).

## 5. Menu <-> prefs and auto-save

- `CreateAppMenus` (3017-3113), run on every `OpenDisplay`: sets `drivertype`
  and clears FLAG_USE_XEM_LIBRARY if no `displaydriver` (3028-3039), then
  `SetNewMenuCheckFromPref(id, flag)` (1792-1807) copies each `prefs.flags` bit
  into the `NewMenu` CHECKED flag (3041-3060) before `CreateMenusA`. So after
  any restart the menu reflects the current `prefs.flags`.
- `UpdatePrefsFlagFromMenu(item, flag)` (1819-1825) copies the Intuition item's
  CHECKED state into `prefs.flags` on MENUPICK (handlers 2283-2392).
- Other writers of `prefs.flags` outside menus: `OpenAppScreen` on WB forces
  FLAG_HIDE_LEDS on and FLAG_PACKET_WINDOW off (2830-2831); `OpenDisplay` clears
  FLAG_USE_XEM_LIBRARY on XEM init failure (3159); `SetLocalEchoBack` from
  telnet ECHO negotiation (1833-1866); `Finger` (above); toolbar with no icons
  clears FLAG_TOOL_BAR (guis.c:1293).
- There is no "Save Settings" menu item. `SavePrefs()` callers:
  - normal quit: `main` 1601, unconditionally after the loop (the next line,
    `prefs.flags = FLAG_HIDE_TITLEBAR;` 1603, is a dead store after the save);
    the error path `clean_exit` skips it;
  - mid-session: Xfer.c:1111 after choosing upload files;
  - `OpenDisplay` screen-open failure (3134); first-time/old-file setup (1230).
- Consequence: `prefs` is simultaneously the live session state and the saved
  state. Any value written into `prefs` for one connection (including the
  server-driven FLAG_LOCAL_ECHO) is persisted at quit, or at the next upload,
  unless restored before those points.
