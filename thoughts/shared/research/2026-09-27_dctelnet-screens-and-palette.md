---
date: 2026-09-27
topic: DCTelnet screen mode, palette and pen usage -- what must change for 256-colour AGA and RTG screens
tags: [screen, palette, pens, aga, rtg, picasso96, cybergraphx, ibmcon, xem, research]
status: final
---

# DCTelnet screens and palette (research, documentary)

Branch `feature/petscii-session-display`. Sources read: all of `src/*.c`, `src/*.h`
(line numbers below), the shipped icons under `build/package/DCTelnet/{SC,WB}Icons`,
`~/Code/ibmcon/ibmcon.device.{bugfixed,original}.asm`, the XEM sources in
`src/third_party/Xem/SOURCE/*.lha` (extracted to scratch), NDK `Autodocs/asl.doc`.
`src/DCTelnet.c` holds 8-bit bytes: searched with `grep -a`.

## 1. Screen mode choice

- `ChooseScreen(firsttime)` DCTelnet.c:759-814.
  - First run defaults (765-768): `PAL_MONITOR_ID|HIRES_KEY`, 640x256, **depth 4**.
  - asl.library >= 38: `ScreenModeRequester()` requesters.c:1012-1078.
    Tags: `ASLSM_Initial{DisplayID,Width,Height,Depth}` from prefs, `DoWidth/DoHeight/DoDepth`,
    `ASLSM_MinWidth 640`, **`ASLSM_MaxDepth 4`** (requesters.c:1051, doc comment 1008).
    No `ASLSM_PropertyFlags/Mask` -> ASL default `DIPF_IS_WB` filter (asl.doc:794-811).
    No `ASLSM_MinDepth`, no filter hook. Result checked with `ModeNotAvailable()` (1062).
  - Fallback (asl < 38): ReqTools `rtScreenModeRequest` with **`RTSC_MaxDepth 4`**,
    `SCREQF_DEPTHGAD|SIZEGADS|GUIMODES` (DCTelnet.c:797-801).
- Stored: `prefs.DisplayID` (ULONG), `DisplayWidth/Height/Depth` (UWORD) prefs.h:13-16.
  An RTG ModeID fits the ULONG and would be stored/compared unchanged; nothing in DCTelnet
  inspects the ID (no `IsCyberModeID`, no `GetDisplayInfoData`). Whether ASL hides RTG
  hi/true-colour modes (min depth 15+) under `MaxDepth 4` or only clamps the depth slider:
  NOT CONFIRMED (asl.doc only says "maximum depth to let the user choose"; needs a run).
- `MENU_SCREEN_MODE` DCTelnet.c:2581-2591: reopens the screen only if ID **or depth**
  changed (width/height change alone does not reopen).
- First-run text says "recommended mode is 640*256*16" DCTelnet.c:1379-1382.
- `prefs.DisplayID == DEFAULT_MONITOR_ID` = "OpenScreen failed last time, ask again"
  sentinel (1368, set at 3328).
- `test-requesters.c:161` drives the requester with depth 4.

## 2. OpenAppScreen() DCTelnet.c:2994-3069

- Workbench mode (`FLAG_USE_WORKBENCH`, 2998): `LockPubScreen(0)` (3027); forces
  `FLAG_HIDE_LEDS` and clears `FLAG_PACKET_WINDOW` (3025-3026). No pens obtained.
- Own screen: `struct NewScreen` + `OpenScreenTags(&newscr, ...)` (3032-3051):
  `Width/Height/Depth` from prefs, `BlockPen = 1`, `Type CUSTOMSCREEN`, `Font &fontAttr`,
  `DefaultTitle`; tags `SA_DisplayID`, `SA_Pens`, `SA_ShowTitle`, `SA_AutoScroll`,
  `SA_Interleaved TRUE`. **Absent:** `SA_Colors32`/`SA_Colors`, `SA_SharePens`,
  `SA_FullPalette`, `SA_LikeWorkbench`, `SA_Depth` tag (depth via NewScreen only).
- Pen array `colorPens[] = {1,4,1,1,6,4,1,0,5,4,1,6,~0}` (DCTelnet.c:253), used when
  `prefs.DisplayDepth >= 3`; depth < 3 passes `&colorPens[12]` = `{~0}` (Intuition defaults)
  (3034). DrawInfo mapping: DETAIL=1 BLOCK=4 TEXT=1 SHINE=1 SHADOW=6 FILL=4 FILLTEXT=1
  BACKGROUND=0 HIGHLIGHTTEXT=5 BARDETAIL=4 BARBLOCK=1 BARTRIM=6.
  **Every UI pen is one of ANSI pens 0-7**, so the UI colours ARE the ANSI colours.
- A second, commented-out `OpenScreenTags` variant 3052-3065 (dead code).

## 3. Palette

- `prefs.color[16]` = 16 x UWORD **RGB4 (12-bit)** prefs.h:22. Defaults
  `color[]` DCTelnet.c:254-256, copied at first run (1394). Order is the ibmcon order:
  0 black, 1 white(DDD), 2 green, 3 yellow, 4 blue, 5 magenta, 6 cyan, 7 red(D00),
  8-15 the bright set (8 = grey 555, 15 = red F00).
- Applied: `LoadRGB4(&scr->ViewPort, prefs.color, 16)` in **`OpenAppWindow()`** (3130),
  i.e. on every window (re)open on the own screen, not only at screen open. Never on WB.
- Only other palette writer in DCTelnet: none. No `SetRGB4/SetRGB32/LoadRGB32` anywhere.
- `ChoosePalette()` 817-839 (`MENU_SCREEN_PALETTE`, 2603): `rtPaletteRequestA` on the
  window's screen (tags `RT_Window win`, `RT_WaitPointer`, DCTelnet.c:740-744), then reads
  back **pens 0-15 only** with `GetRGB4` (833). ReqTools edits the live screen colours;
  on a deeper screen it will offer all 2^depth pens (NOT CONFIRMED: reqtools palette
  requester's depth limit and whether it writes 12- or 24-bit).
- Menu item disabled on WB (3092), enabled on own screen (3128).
- Palette persistence: prefs file is a raw `Write(&prefs, sizeof)` (583); load accepts
  anything >= 252 bytes (1346). Per-site prefs (`site_prefs.c`) are a raw dump with an
  **exact-size** check (`SITE_PREFS_FILE_SIZE`, site_prefs.h:37, site_prefs.c:88);
  `SitePrefs_DisplayDiffers` compares `color[]` with `memcmp` -> windows-only reopen
  (site_prefs.c:71). `test/test_site_prefs.c:87` assumes `color[3] = 0x0F00` (RGB4).
- Workbench: no `ObtainPen/ObtainBestPen/ReleasePen` anywhere; raw pen numbers are used.

## 4. DCTelnet's own drawing (pens, depth, planar assumptions)

| Site | Draws | Pen |
|---|---|---|
| DCTelnet.c:1183-1192 `LEDs()` | screen title-bar LED, `scr->RastPort` | EraseRect (pen 0), `SetAPen 15` connected |
| DCTelnet.c:1733-1741 | receive-activity LED | `SetAPen 10`, EraseRect |
| DCTelnet.c:2853-2857 | "connecting" LED | `SetAPen 11` |
| DCTelnet.c:2479-2485 `MENU_DISABLE_LEDS` | repaint LED area as title bar | `SetAPen 1` (assumes BARBLOCKPEN=1) |
| DCTelnet.c:3189-3190 | packet window separator line | `SetAPen 1` |
| DCTelnet.c:3270-3271 | Quit menu item text | `FrontPen = 15` if `prefs.DisplayDepth > 1` (prefs depth, not the real screen; also on WB) |
| DCTelnet.c:3039 | `newscr.BlockPen = 1` | raw |
| DCTelnet.c:3084-3085 | `newWin.Detail/BlockPen = 255` | default (OK) |
| DCTelnet.c:2742 | scrollback resize clear | `SetRast(rp, 0)` raw pen 0 |
| DCTelnet.c:1196-1228 SpeedTest | text via console only (`ConWrite` SGR reset) | none direct |
| guis.c:1091,1112-1113 `RefreshListView` | scrollback text | `dri_Pens[TEXTPEN]`, EraseRect (backfill pen 0) |
| guis.c:1623-1641 toolbar | labels + two separator lines | `dri_Pens[TEXTPEN/SHINEPEN/FILLPEN]` |
| Xfer.c:605-607, 1022, 1058-1070 | transfer window gauge/frame | `dri_Pens[...]` |
| connect.c:92, guis.c:390/395/874 | GadTools `DrawBevelBox` | VisualInfo/DrawInfo |
| requesters.c:947-948 | ASL font requester preview | `ASL_FrontPen 1`, `ASL_BackPen 0` |
| Xem_wrapper.c:89, Xfer.c:706/750 | `DisplayBeep(scr)` | flashes colour 0 |

- Toolbar icons: `GetDiskObjectNew("PROGDIR:SCIcons/..."` or `WBIcons/`) guis.c:1544-1558,
  their `do_Gadget` used directly as `GFLG_GADGIMAGE` gadgets (no `LayoutIcon`/remap).
  All 14 shipped icons: classic planar `Image`, 55x30, **depth 3, PlanePick 7** (parsed from
  the .info files) -> they render with pens 0-7 = the ANSI palette on the own screen and
  the WB palette on WB. Intuition draws planar Images on RTG, so no crash, only colour.
- Window backgrounds: Intuition's default layer backfill is pen 0 on every window
  (terminal, toolbar, packet, scrollback, requesters). Pen 0 is also ANSI black and the
  console's clear/background pen -> pen 0 is shared by UI and ANSI whatever SA_Pens says.
- No direct BitMap/plane access, no `ScrollRaster`/`ClipBlit`/`BltBitMap` in DCTelnet
  itself. Only chip data: wait pointer sprite DCTelnet.c:393 (sprite, fine on RTG).
- `drawInfo = GetScreenDrawInfo(scr)` DCTelnet.c:3335, freed 3613.

## 5. Console / renderer and its colour contract

- `OpenDisplay()` DCTelnet.c:3375-3438: WB -> `console.device` unit `CONU_SNIPMAP`;
  own screen -> `ibmcon.device`, unit **2** if `FLAG_JUMP_SCROLL` else `CONU_CHARMAP` (1)
  (3395-3403); `io_Data = win`, `CONFLAG_DEFAULT`. No colour parameters are passed.
- ibmcon.device (`~/Code/ibmcon/ibmcon.device.bugfixed.asm`): SGR 30-37 -> pen
  `n-30`, 40-47 -> pen `n-40` (1437-1475); unit >= 1 swaps pens 1 and 7; bold = pen+8 on
  depth >= 4; reset/erase/clear use pen 0 (`SetRast 0` at 904, `clr con_BgPen` 941/1406).
  Pens are fixed at 0-15, no pen table. **Depth dispatch 802-835 matches depth 1,2,3,4
  only; depth 5-8 (and RTG 15/16/24) falls through into `.depth1` = monochrome, colours
  ignored** (same in the original dump, lines 350-365). Depth is read raw from
  `wd_WScreen->ViewPort.RasInfo->BitMap->bm_Depth`, not `GetBitMapAttr(BMA_DEPTH)`.
  The shipped `build/package/DCTelnet/Devs/ibmcon.device` md5 9c893609... differs from
  `~/Code/ibmcon/ibmcon.device` ec843897... (which build ships is not confirmed).
- console.device (WB): SGR 30-37 -> WB pens 0-7 as-is; DCTelnet cannot give it a pen map.
- XEM (`Xem_wrapper.c:111-172`, `InitializeXemLibrary`): passes `xem_window = win`,
  `xem_font = ansiFont`, **`xem_screendepth = scr->BitMap.Depth`** (130, raw embedded
  BitMap, not `GetBitMapAttr`). XEM sources (third_party, version vs shipped v4.1 binary
  NOT CONFIRMED): SGR -> pen `n-30`/`n-40` raw, background pen checked against
  `1<<xem_screendepth` (VT340_CSIfuncs.c:878-897, IBM_CSIfuncs.c:436); IBM emulator
  blinks by `SetRGB4` on the screen's colour (IBM_Suppfuncs.c:249-258).
- PETSCII path (petscii_dispatch.c:40-57) converts C64 colours to SGR sequences, so it
  inherits the console's pen mapping; no pen numbers of its own.

## 6. RTG support present today

None. No cybergraphics.library / Picasso96API.library opens, no `GetCyberMapAttr`,
`IsCyberModeID`, `GetBitMapAttr`, `SetRGB32`, `LoadRGB32`, `SA_Colors32`, `ObtainBestPen`
anywhere in `src/` (grep, case-insensitive; only `thoughts/spot/todos.md:19-30` mentions it).

## Change sites

| file:line | Now | Must change because |
|---|---|---|
| requesters.c:1051 (+1008 doc) | `ASLSM_MaxDepth 4` | blocks depth 5-8 and likely RTG modes |
| requesters.c:1037-1052 | default `DIPF_IS_WB` filter, no min depth | decide which RTG/AGA modes are legal |
| DCTelnet.c:800 | `RTSC_MaxDepth 4` (ReqTools fallback) | same limit on the fallback path |
| DCTelnet.c:765-768 | first-run default depth 4 | default for a 256-colour screen |
| DCTelnet.c:1379-1382 | first-run text "640*256*16" | text states the 16-colour limit |
| DCTelnet.c:2581-2591 | reopen only on ID/depth change | fine for RTG; width/height change is ignored (pre-existing) |
| DCTelnet.c:3032-3051 | NewScreen + tags, no `SA_Colors32`/`SA_FullPalette`/`SA_SharePens` | UI and ANSI colours must be set on separate pens at open |
| DCTelnet.c:253, 3034 | `colorPens` = ANSI pens 0-7; depth<3 -> defaults | UI pens must move off the ANSI pens (16+ on depth >= 5) |
| DCTelnet.c:254-256 | 16 x RGB4 defaults | 12-bit; needs 24-bit values and UI colours of its own |
| prefs.h:22 | `UWORD color[16]` RGB4 | 12-bit only; no room for UI colours; raw-dump layout |
| DCTelnet.c:583, 1346 | raw prefs write / `< 252` old-file check | any layout change breaks existing DCTelnet.Prefs |
| site_prefs.h:37, site_prefs.c:71,88 | exact-size raw dump, `memcmp color` | layout change invalidates saved per-site files |
| test/test_site_prefs.c:87 | `color[3] = 0x0F00` | RGB4 assumption in the test |
| DCTelnet.c:3130 | `LoadRGB4(vp, prefs.color, 16)` per window open | 12-bit; must load ANSI + UI pens (LoadRGB32/SetRGB32, V39) |
| DCTelnet.c:817-839 | reqtools palette, `GetRGB4` pens 0-15 | reads 12-bit, only 16 pens; user can also edit UI pens |
| DCTelnet.c:1189, 1736, 2855 | LED pens 15/10/11 | raw ANSI pens; OK only while ANSI stays 0-15 |
| DCTelnet.c:1185-1186, 1741 | EraseRect -> pen 0 on title bar | assumes pen 0 LED socket; title bar is BARBLOCKPEN |
| DCTelnet.c:2483 | `SetAPen 1` to repaint title bar | must be `dri_Pens[BARBLOCKPEN]` |
| DCTelnet.c:3189 | packet line `SetAPen 1` | raw UI pen |
| DCTelnet.c:3270-3271 | Quit `FrontPen 15`, gated on prefs depth | raw pen, wrong depth source, also hits WB |
| DCTelnet.c:3039 | `newscr.BlockPen = 1` | raw UI pen |
| DCTelnet.c:2742 | `SetRast(scrollback, 0)` | raw pen 0 instead of BACKGROUNDPEN |
| guis.c:1544-1558 | depth-3 planar icons, no remap | drawn with ANSI pens 0-7; need UI pens or remap |
| Xem_wrapper.c:130 | `scr->BitMap.Depth` | raw struct read; RTG needs `GetBitMapAttr(BMA_DEPTH)` |
| ibmcon.device (external) 802-835 | depth > 4 -> mono | **blocks every deeper screen**; fix lives in ibmcon, not DCTelnet |
| ibmcon.device 904/941/1406 | background/clear = pen 0 | pen 0 stays ANSI black -> shared with every window backfill |
| DCTelnet.c:3395-3403 | unit 1 / 2, no pen parameters | only lever over ibmcon colour is the unit number today |

## Constraints that shape any design (facts, not a plan)

- All three renderers map SGR n straight onto pen n (0-7, +8 bold for ibmcon). The ANSI set
  therefore has to stay on pens 0-15 unless ibmcon/XEM gain a pen table; the UI pens
  (SA_Pens/DrawInfo) are the ones that can move (pens 16+ exist only at depth >= 5).
- Pen 0 is the window backfill for every Intuition window and ibmcon's background: it
  cannot be separated from ANSI black without a backfill hook or an ibmcon change.
- On AGA the mouse pointer uses colours 17-19 (sprite bank 16-31); UI pens placed there
  would be recoloured by pointer prefs.
- On WB, DCTelnet owns no pens; console.device and the icons use WB pens 0-7.

## Not confirmed

- ASL's treatment of RTG modes under `ASLSM_MaxDepth 4` (hidden vs depth-clamped).
- ReqTools palette requester limits on 8-bit / RTG screens.
- Which ibmcon build ships in the package (md5 differs from `~/Code/ibmcon`).
- Whether XEM third_party sources match the shipped `xemvt340.library` v4.1.
- What `scr->BitMap.Depth` holds on P96/CGX hi/true-colour screens (not run).
