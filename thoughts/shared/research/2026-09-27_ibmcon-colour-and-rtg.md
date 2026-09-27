---
date: 2026-09-27
topic: ibmcon.device colour, pen and depth handling -- what must change for 256-colour AGA and RTG screens
tags: [ibmcon, rtg, aga, picasso96, cybergraphx, pens, sgr, research]
status: final
---

# ibmcon.device: colour and RTG trace

Source traced: `/Users/spot/Code/ibmcon/ibmcon.device.bugfixed.asm` (3835 lines; the
README calls it `ibmcon.device.readable.asm` -- that file does not exist, the bugfixed
file is it). Line numbers below are in that file. Cross-checked against the untouched
dump `ibmcon.device.original.asm` where noted.

## Headline

1. **On any screen deeper than 4 planes ibmcon renders MONOCHROME today.** The depth
   switch in HandlerProc (802-839) handles depth 1, 2, 3, 4 and *falls through into the
   depth-1 case for everything else* (5, 6, 7, 8, and RTG 15/16/24/32). Depth-1 sets
   `con_FixedPen=1` (all SGR 30-49 ignored, 1438/1468), `con_BoldPens=0` (SGR 1 ignored),
   `con_DefaultPen=1`. The comment ".depth4plus: 16+ colours" is wrong: the branch is
   taken only for depth == 4 (after four `subq #1`, `beq`). Prediction, not observed on
   hardware: a 256-colour or RTG DCTelnet window shows pen 1 on pen 0, no colours.
2. SGR colours are **raw pen numbers 0-15** (`param-30`, `param-40`, bold = `pen|8`).
   There is no indirection; the 16 ANSI colours must sit on hardware pens 0-15, which on a
   deeper screen are the Workbench/UI pens.
3. **No client hook exists** for a pen table: only `io_Data` (Window) and the unit number
   (0 vs >=1: pen 1<->7 swap) are read. OpenDevice flags (D1) are ignored, BeginIO accepts
   only CMD_WRITE, the CSI table has no private sequence.
4. **No direct bitmap/plane/custom-chip access anywhere.** Everything goes through
   graphics.library (SetAPen/SetBPen/SetDrMd/SetRast/RectFill/ScrollRaster/Move/Text/
   SetSoftStyle), which Picasso96/CGX patch. The only depth reads are `bm_Depth` via
   `Screen->ViewPort.RasInfo->BitMap` (805-807, 1481-1483). No `rp->Mask`, no
   Planes[]/BytesPerRow, no BltClear/BltPattern/EraseRect, no ClipBlit after fix 3.
5. The cursor is a **COMPLEMENT RectFill** (2349-2384): on a deeper or chunky screen it
   produces a palette-dependent colour, not a predictable inverse.

## 1. How the device learns the window, and what it caches (item 5)

| Line | What |
|---|---|
| 3031 | `g_UnitNum` = unit from OpenDevice (D0). D1 flags are dropped (D1 reused at 552). |
| 3032-3034 | `g_Window` = `io_Data`; `g_RastPort` = `window->RPort`. Cached for the device lifetime. |
| 797 | `con_RastPort` = copy of g_RastPort (both are used interchangeably). |
| 802-839 | Depth read ONCE at handler start -> DefaultPen/BoldPens/FixedPen cached in con. |
| 1478-1483 | Depth read again LIVE for every SGR 40-47 (validation). |
| 906, 998, 1082, ... | `rp_Font` and tf_XSize/YSize/Baseline are read live on every use (never cached). |
| 907-933, 998-1007 | Window width/height read only in ConInit and CSI t -> con_Rows/Cols/WidthPx (no resize handling). |
| 3617, 3652 | `g_LibVersion` = 0: graphics/intuition opened with version 0, so any V39 call (GetBitMapAttr, ObtainBestPen, SetRPAttrs, SetABPenDrMd) needs a version check or a raised open version. |

Pens are NOT cached outside con; the device relies on the pens/drawmode it last set staying
in the shared RastPort between writes (FlushText at 2439-2463 sets no pen). The handler
process draws into the CLIENT's RastPort with no LockLayer/cloned RastPort: any client
drawing into the same RastPort that changes APen/BPen/DrMd will corrupt ibmcon's text
colours until the next SGR. (Relevant if DCTelnet draws its own things in that window.)

## 2. Depth dispatch (items 3, 1)

`HandlerProc` 802-839 (con defaults are written through the handler frame `$56(A7)` =
con_DefaultPen/BoldPens/FixedPen):

| Depth | DefaultPen | BoldPens | FixedPen | Effect |
|---|---|---|---|---|
| 1 (816-821) | 1 | 0 | 1 | mono, SGR colour + bold ignored |
| 2 (822-828) | 1 | 1 | 1 | colours ignored, bold = pen 3 (1531) |
| 3 (829-833) | 1 | 0 | 0 | SGR colours, no bold |
| 4 (834-839) | 1 | 1 | 0 | SGR colours, bold = pen\|8 |
| **anything else** | 1 | 0 | 1 | **falls into .depth1 -> mono** |

Must change because: depth 8 AGA and every RTG depth take the mono path. For RTG the depth
source itself is also suspect: Picasso96/CGX document that `struct BitMap` fields of RTG
bitmaps must not be read directly; `GetBitMapAttr(bm, BMA_DEPTH)` (V39) is the documented
query. NOT CONFIRMED what `bm_Depth` holds on P96/CGX hi/truecolour screens (looked only
in this source; no P96/CGX headers or docs in the repo).

## 3. SGR `CSI ... m` (item 1) -- `Csi_m_SetGraphics` 1326-1571

- Jump table 1352-1401 (params >= 50 ignored, 1346). Supported: 0,1,3,4,7,23,24,30-37,
  39,40-47,49. Not supported: 2, 5, 22, 27 (reverse cannot be switched off except by 0).
- **Reset (0)** 1403-1411: `con_FgPen = con_DefaultPen` (always 1), `con_BgPen = 0`.
- **Bold (1)** 1412-1417: only if `con_BoldPens`; sets AttrFlags bit4.
- **Foreground** 1437-1466: `con_FgPen = param-30`; 39 -> DefaultPen (1446-1450).
  Unit >= 1 (1453-1466): swap pen 1 <-> 7. Inference (not verified against DCTelnet's
  palette code): IBM 1=red/7=white, so the client palette has white on pen 1 (= DefaultPen)
  and red on pen 7.
- **Background** 1467-1507: `con_BgPen = param-40`; 49 -> 0. Validation 1478-1489:
  `moveq #0,D2; bset D1,D2` with D1 = bm_Depth, i.e. number of pens = 1<<depth; pen >= that
  -> 0. `bset Dn,Dm` is modulo 32, so depth 32 gives D2=1 and every bg pen >= 1 is
  rejected. Same unit >= 1 swap 1495-1507.
- **fg == bg guard** 1515-1523: if equal and no bold pens, reset to DefaultPen/0.
- **Bold apply** 1527-1540: FixedPen -> pen 3 (1531); else `if pen <= 7: bset #3` (pen+8,
  1534-1538). The result is WRITTEN BACK into con_FgPen (1540), so bold persists in the
  stored pen until the next colour/reset.
- **Apply** SetAPen 1541-1546, SetBPen 1551-1555, SetSoftStyle 1560-1567.
- `Csi_s` 1138-1150 saves Row/Col/Attrs/Fg/Bg/Mask/Modes; `Csi_u` 1156-1189 restores
  and re-applies SetAPen(SavedFg) 1170-1175, SetBPen 1176-1179, SetSoftStyle 1180-1186.

No `AND #7` / `AND #15` masks on pens exist; the only `#$F` is the CSI digit decode
(2751, not colour). Pen arithmetic is `param-30`, `param-40`, `bset #3`, `cmp #7`, `#3`,
`#1`/`#7` swap, `#0` default bg, `#1` default fg.

Could pens come from a client remap table? Yes, cleanly: keep con_FgPen/con_BgPen as
LOGICAL ANSI indices 0-15 (all the arithmetic above already works in that space) and
translate logical -> hardware pen at the 7 points where a pen leaves the device (SetAPen x4,
SetBPen x3, SetRast x2; table in section 5). No existing field carries such a table.
Candidate carriers (none exists today): a private io_Command (needs DevBeginIO 3129-3132
and HandlerProc 852-857 to accept it -- not reachable from the remote byte stream), OpenDevice
flags D1 (ignored today, 536/552), or a private CSI (reachable by any BBS -- unsafe).

## 4. Drawing primitives (item 2)

| Primitive | Lines | Pens / mode | RTG / deep-screen note |
|---|---|---|---|
| SetRast(0) | 903-904 (ConInit) | pen 0 hard-coded | clears to hardware pen 0, not the ANSI background |
| SetRast(BgPen) | 2656-2660 (ClearScreenHome, FF and CSI 2J) | con_BgPen raw | raw pen |
| RectFill | 2051-2052 (ClearRect) | SetAPen(BgPen) 2032-2036, restore FgPen 2053-2057 | raw pens; replaced ClipBlit minterm $00 of the original (original.asm 1566, D6=0) |
| RectFill COMPLEMENT | 2347-2384 (ToggleCursor) | SetDrMd(COMPLEMENT) 2349-2350, RectFill 2381, SetDrMd(JAM2) 2383-2384 | see below |
| ScrollRaster | 1119 (@), 1623 (S), 1668 (T), 1728 (P), 1799 (M), 1869 (L), 2549 (down/LF), 2630 (up) | vacated area filled by gfx with rp->BgPen | follows whatever SetBPen set; no own pen |
| Move + Text | 2439, 2451 / 2463 (FlushText) | JAM2, or JAM2\|INVERSVID (2445) for reverse | uses rp APen/BPen as last set |
| SetSoftStyle | 1186, 1567 | enable mask 5 (underline, italic) | no pen effect |
| DisplayBeep | 2900 | screen flash | no pen effect |

Not used anywhere: ClipBlit (LVO defined at 275, no call), EraseRect, BltClear, BltPattern,
BltBitMap, WritePixel, SetWriteMask/rp->Mask, Planes[], BytesPerRow, custom-chip registers.
The depth read path `wd_WScreen -> sc_ViewPort.vp_RasInfo -> ri_BitMap -> bm_Depth`
(EQUs 290-296) is the only struct-level bitmap access.

**Cursor (ToggleCursor 2336-2387).** COMPLEMENT inverts all bits of rp->Mask (default $FF,
ibmcon never touches it): on 4 planes pen p -> 15-p (the 16 ANSI pens map among
themselves), on 8 planes p -> 255-p (a UI/unallocated pen: arbitrary colour), on RTG
hi/truecolour the driver inverts the pixel value (RGB complement) -- NOT CONFIRMED per
driver. It is self-inverse, and ConWrite removes it on entry (2696) and redraws it at the
end (3014), so it still erases correctly; only its colour is wrong. With ANSI colours on
non-contiguous pens the XOR result is not an ANSI colour. Must change because: a
deterministic cursor needs either an explicit pen swap (redraw the cell with fg/bg
exchanged) or a pen-pair-aware XOR.

## 5. Soft style and SO/SI (item 4)

SetSoftStyle (1180-1186, 1560-1567) uses the style word only; italic/underline do not
touch pens. SO/SI (2954-2958, applied 2970) ORs $80 into the character code; no pen
involvement. Neither needs to change for colour.

## 6. Change-site table

| Line(s) | Routine | What it does now | Must change because |
|---|---|---|---|
| 802-839 | HandlerProc | depth 1/2/3/4 switch, all other depths fall into mono | depth 8 and RTG become mono; needs a ">= 4" (or table-supplied) path, depth via GetBitMapAttr on RTG |
| 805-807 | HandlerProc | reads bm_Depth from ViewPort RasInfo BitMap | not the documented query on RTG bitmaps |
| 903-904 | ConInit | SetRast(rp, 0) | pen 0 is the UI background pen, not ANSI black |
| 940-941 | ConInit | FgPen = DefaultPen(1), BgPen = 0 | logical defaults are fine only if mapped at output |
| 966-974 | ConInit | SetAPen(FgPen), SetBPen(BgPen) | raw pen out -> needs logical->hardware mapping |
| 1170-1179 | Csi_u_RestoreCursor | SetAPen/SetBPen with saved pens | raw pen out |
| 1403-1406 | Csi_m reset | Fg=DefaultPen, Bg=0 | logical, OK if mapped |
| 1443-1450 | Csi_m fg | pen = param-30 (39 -> default) | logical index; OK if mapped |
| 1453-1466, 1495-1507 | Csi_m | unit >= 1 swaps pens 1 and 7 | palette-order hack; a client pen table subsumes it |
| 1473-1492 | Csi_m bg | pen = param-40, validated against 1<<bm_Depth (bset mod 32) | depth 32 rejects every bg; validation is of hardware pens, meaningless for logical indices |
| 1515-1523 | Csi_m | fg==bg guard | logical, OK |
| 1531 | Csi_m bold | 4-colour screens: bold = pen 3 | raw hardware pen |
| 1534-1540 | Csi_m bold | pen <= 7 -> pen\|8, stored back | assumes bright colours on pens 8-15; OK as logical index |
| 1541-1555 | Csi_m apply | SetAPen/SetBPen raw | raw pen out |
| 2032-2036, 2053-2057 | ClearRect | SetAPen(BgPen) / SetAPen(FgPen) | raw pen out |
| 2349-2384 | ToggleCursor | COMPLEMENT RectFill | colour depends on depth/palette; wrong on 8-bit and RTG |
| 2656-2660 | ClearScreenHome | SetRast(BgPen) | raw pen out |
| 1119...2630 (8x) | ScrollRaster sites | fill with rp->BgPen | no change if SetBPen is mapped |
| 3617, 3652 | Open*Lib | OpenLibrary version 0 | V39 calls need a version gate |
| 3129-3132, 852-857 | DevBeginIO / HandlerProc | only CMD_WRITE (+ private CMD_DIE $7FF0) | no channel for a pen table |

## 7. Build (item 6)

- `vasmm68k_mot -Fhunkexe -kick1hunks -nosym -o <scratch>/ibmcon.device.rebuilt
  ibmcon.device.bugfixed.asm` assembles cleanly (vasm 2.0d, motorola 3.19d; Segment0 7762,
  Segment1 16, Segment2 64, Segment3 320 bytes) and is **byte-identical** to
  `/Users/spot/Code/ibmcon/ibmcon.device` (8572 bytes, md5 ec843897507115f01812119cc76f9126).
  So the ibmcon folder's binary IS the bugfixed build, not the shipped original.
- Other ibmcon binaries on disk, all "$VER: ibmcon.device 1.4 (Mar 9 1998)", all different:
  - `dctelnet-petscii-recovered/build/package/DCTelnet/Devs/ibmcon.device` = Aminet DCTelnet
    1.6 original (commit 2a46eb2), 8576 bytes, md5 9c893609967522c9312bd5dccfac623e, code
    hunk 7768 bytes. Same file in `~/Code/DCTelnet-PETSCII`.
  - `~/Downloads/DCTelnet-PETSCII/DCTelnet/Devs/ibmcon.device` (the owner's FS-UAE test
    mount), 8548 bytes, md5 ecf36fc04f71f58d4cb9fa07fafdf0c0, code 7740 bytes. Matches
    neither the Aminet file nor the bugfixed build. Provenance UNKNOWN.
- The untouched dump `ibmcon.device.original.asm` reassembles to 8544 bytes (8560 with
  `-no-opt`), NOT identical to the Aminet binary: first difference at code offset $41,
  Aminet `lea 1.L,A1` (43F9 00000001) vs dump `lea 1.W,A1` (43F8 0001). Encoding-only
  differences are the likely cause; a full semantic diff of the two was NOT done.
- Consequence: there is no byte-exact regression baseline against the shipped binary;
  a changed build can only be diffed against the bugfixed build.

## 8. Not confirmed / where looked

- Contents of `bm_Depth` on P96/CGX hi/truecolour screens, and how each RTG driver
  implements COMPLEMENT and rp->Mask on chunky bitmaps: no P96/CGX docs or headers searched
  (none in either repo). Needs the vendor autodocs or a probe.
- Which unit DCTelnet opens ibmcon with and its palette layout: `grep OpenDevice` over
  `dctelnet-petscii-recovered/src` (excluding third_party) found no call; the 1<->7
  interpretation above is inferred from ibmcon alone.
- ScrollRaster filling with BgPen is standard graphics.library behaviour (and what fix 3
  relies on); not re-verified against the autodoc here.


## Correction (2026-09-27, main session)

"No OpenDevice call in the recovered src" is wrong: `src/DCTelnet.c` opens the console with `OpenDevice(devName, unitNumber, writeConsoleReq, CONFLAG_DEFAULT)` (~line 3221; unit 1, or 2 with Jump Scroll). The search missed it because DCTelnet.c holds 8-bit bytes and plain grep skips it as binary -- use `grep -a`.
