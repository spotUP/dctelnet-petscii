---
date: 2026-09-27
topic: "RTG and 256-colour screens: pen table in ibmcon, ANSI pens of their own"
tags: [dctelnet, ibmcon, rtg, aga, picasso96, palette, pens]
status: final
---

# RTG and 256-colour screens -- plan

Research: `thoughts/shared/research/2026-09-27_ibmcon-colour-and-rtg.md`,
`2026-09-27_dctelnet-screens-and-palette.md` (docs branch). ibmcon source:
`~/Code/ibmcon` (git since ef067a6, `ibmcon.device.bugfixed.asm`).

## Decisions (owner, 2026-09-27)

- R-D1 **Pen table in ibmcon.** ANSI colours stay logical 0-15 inside ibmcon and are mapped
  to real pens that DCTelnet hands over through a private IO command (not reachable from
  the byte stream, so no BBS can change it).
- R-D2 First delivery covers **AGA 32-256 colours, RTG 8-bit, RTG 15/16/24/32-bit and the
  Workbench with real ANSI colours**.
- R-D3 **Workbench uses ibmcon too**, with 16 pens obtained from the Workbench palette.
  Consequence accepted: console.device's mouse copy of terminal text goes; the scrollback
  window still copies.
- R-D4 ibmcon is a git repo; changes on a branch there.
- R-D5 Test rig: the owner's FS-UAE, which has RTG.

## The invariant

> The 16 ANSI colours are drawn with 16 pens reserved for them; the AmigaOS UI (title bar,
> menus, gadgets, requesters, tool bar, window backgrounds) keeps its own pens and colours.
> Where the screen has fewer than 32 colours this is impossible and today's shared pens
> 0-15 stay (no regression).

## Pen layout

| Screen | UI pens | ANSI pens | How |
|---|---|---|---|
| own, depth <= 4 | 0-15 shared | 0-15 (today) | table = identity (+ today's unit swap) |
| own, depth >= 5 (AGA, RTG 8-bit) | 0-7, Intuition defaults | 16 fixed pens from 8 up, skipping 16-19 (AGA pointer: 17-19) | SA_SharePens, LoadRGB32 for those 16 |
| own, RTG 15-32 bit | same as above | same as above | pens are emulated through the colour map; same code |
| Workbench | the Workbench's | 16 x ObtainBestPen(exact) | released on close |

## Phases

R1. **Baseline check of the bug-fixed ibmcon** (manual, owner): DCTelnet today with
    `~/Code/ibmcon/ibmcon.device` (bug-fixed 1.4, never shipped) instead of the Aminet one,
    on the current 16-colour screen -- ANSI boards look the same. Needed because every
    later ibmcon build is based on the bug-fixed source.
R2. **Probe** `tools/rtgprobe` (vbcc, runs on the Amiga): for a screen mode picked in the
    ASL requester it logs to `PROGDIR:rtgprobe.txt` bm_Depth, GetBitMapAttr(BMA_DEPTH/
    FLAGS), whether the mode is RTG, and what a COMPLEMENT RectFill with a write mask does to
    two known pens (ReadPixel before/after). Run by the owner on AGA 256, RTG 8, RTG 16/24.
    Decides the cursor (R3) and the depth query.
R3. **ibmcon 1.5** (branch in ~/Code/ibmcon):
    - graphics opened at V39 when present; depth from GetBitMapAttr there, bm_Depth below.
    - colour mode for every depth >= 4 (today only == 4; deeper screens fall into mono).
    - private command `IBMCMD_SETPENS` (io_Command outside CMD_* range, io_Data -> 16 pens):
      logical pens mapped at the 9 output sites (SetAPen x4, SetBPen x3, SetRast x2); the
      unit 1<->7 swap and the 1<<depth background check apply only without a table.
    - initial clear and every erase use the mapped ANSI background, not pen 0.
    - cursor from R2's result: COMPLEMENT with write mask fg^bg where it swaps exactly,
      otherwise an explicit fg-pen block.
    - version string 1.5; README lists the changes.
R4. **Palette and prefs**: `ULONG ansiRGB[16]` (0x00RRGGBB) appended to PrefsStruct
    (old files load; missing values come from the RGB4 color[16]); site files 'DCS3' with
    DCS2/DCS1 still read; Screen group includes it; LoadRGB32 on V39 (LoadRGB4 below);
    Screen Palette edits the 16 ANSI pens.
R5. **Own screen**: screen-mode requester up to depth 8 and RTG modes; SA_SharePens +
    SA_FullPalette; UI pens 0-7 (SA_Pens = Intuition defaults) with their own colours;
    ANSI pens per the table above, sent to ibmcon after OpenDevice; DCTelnet's raw pens
    (LEDs 15/10/11, title-bar repaint 1, packet line 1, Quit item 15, BlockPen 1,
    SetRast 0) take DrawInfo or ANSI-table pens; XEM depth via GetBitMapAttr (XEM itself
    keeps raw pens: noted limit, third-party library).
R6. **Workbench**: ibmcon on the Workbench window with 16 obtained pens; released in
    CloseDisplay.
R7. **Verify + land**: host tests (pen layout, RGB4->RGB32, DCS3), builds, review pass,
    install + verify on the FS-UAE drive, owner tests AGA 256 / RTG 8 / RTG 16-24 / WB;
    PRs: ibmcon changes first (its provenance -- a disassembled 1998 freeware binary --
    stated in the PR), then DCTelnet.

## Verification

Automated: host tests for the pen-layout function (depths 1-8, RTG, pointer pens skipped),
RGB4 -> RGB32 conversion, DCS3/DCS2/DCS1 files. ibmcon has no host tests (68k assembly);
its proof is R7's manual matrix.

Manual (owner, FS-UAE, `~/Downloads/DCTelnet-PETSCII`), per screen type: an ANSI board
shows its colours, the title bar/menus/requesters keep Workbench-like colours, the cursor
is visible and erases cleanly, PETSCII boards still work, and (WB) other programs' colours
are unchanged while DCTelnet runs and after it quits.
