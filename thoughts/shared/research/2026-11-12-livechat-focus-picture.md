---
date: 2026-11-12
topic: LiveChat door picture hides on log-border click, returns on input click
status: NOT REPRODUCED on host; no fix, no commit
worktree: /Users/spot/Code/DCTelnet/dctelnet-focuspic (branch fix/livechat-focus-picture = 43266a8, vtcon 5fe845d6)
---

# LiveChat focus-click picture hide: host reproduction attempt

## Verdict

The board's exact click bytes do NOT reproduce the bug in the host harness (tests/test_term_engine.c), on any
screen configuration tried. No root cause found in DCTelnet's vtbuiltin.c or the vtcon engine C code from the host
side. Nothing committed; no Amiga build run (no fix to build).

## Test written (uncommitted, in the worktree)

`test_kfp_the_door_focus_clicks_leave_its_picture_shown` (helpers `kfp_run`, `kfp_run_v`, `kfp_put`,
`kfp_pens_changed`, byte literals `kfp_border_click` / `kfp_input_click` generated verbatim from the board's
RAW JSON in livechat-focus-bytes.md). It:

1. draws the door's input-focused screen (input-click bytes), uploads a 100x75 RGBA picture (a=t,f=32,i=1),
2. places it exactly as the door: `ESC7 ESC[8;25H ESC_Ga=p,i=1,c=52,r=10,p=1,C=1,q=2 ESC\ ESC8`,
3. replays border click, input click, border click, input click, and after each asserts:
   no text cell painted on covered picture cells, no picture pixel's pen changed in the framebuffer
   (cols 25-76, rows 8-17), and no pen used by the picture released or recoloured (obtainLive / obtainRgb).

Configurations, all PASS (pixels changed 0, pens recoloured 0 every step):
- depth 4 (BMF_STANDARD), 5 (12 free pens, picker), 6 (44), 7 (108), 8 (236), 24 (unlimited)
- cursor hide/show added (`ESC[?25l` after border click, `ESC[23;3H ESC[?25h` after input click), depth 5 and 8
- cursor parked inside the picture (`ESC[12;30H ESC[?25h`) after the border click, depth 8
- click bytes fed in 61-byte VtWrite chunks (TCP split), depth 8
- picture with transparent strips (alpha 0 on left 10 columns and top 5 rows), depth 5 and 8: text is painted
  under the transparent cells (expected, 106 cells) but the final frame is unchanged.

Also fixed in the harness: the `Text()` mock strcat'd into `calls[8192]` unchecked; a whole-screen repaint
overflowed it (macOS `__chk_fail_overflow` trap). Now bounded like `call()`.

## Findings that narrow the cause

- Each click does repaint the picture whole (315-666 picture runs per click, depending on depth): the border
  writes at cols 23/80 damage whole rows 8-17, so VtDraw redraws those rows' text (VtRowCover skips covered cells)
  and VtDrawImages blits the picture again. On the host the blit always lands with the right pens.
  So (a) "the row redraw drops the picture" does not happen in the C logic; the picture is redrawn after BOTH clicks.
- (b) palette theft does not happen in the host model: xterm 255 and 204 are already in vtTc from the initial
  screen, no picture pen is released or reassigned on either click (VtPicSlot keeps the slot by key; VtPicSweep
  only runs when vt_image_frees changes, and no click frees an image).
- The engine's 68k asm (vtengine_68k.s, VT_ASM) is not in src/Makefile, so the Amiga engine runs the same C as the
  host; an engine-asm difference is ruled out.

## Where the cause must be (unverified; needs the owner's setup)

Only Amiga-only code differs from the host model:
1. the real drawing back end: term_* direct-bitmap writes / blitter, VtImgMapRow (inline 68k asm,
   src/vtbuiltin.c ~line 488), VtImgBlit, and the covered (RastPort) path VtLayersLock/VtCovered (src/vtbuiltin.c
   ~1056-1100). A full-row redraw that blanks rows 8-17 then fails or is clipped in the blit would look exactly
   like "gone until the next full redraw";
2. the owner's real screen mode and pen budget (unknown: depth, RTG or native, free pens);
3. the real picture (chat.png via f=100) through vtcon 5fe845d6's new 255-entry undithered palette cut, which the
   host test does not exercise with a real PNG;
4. bytes the board capture may not include (blessed's own cursor writes go through program.output; the capture
   shows none).

Next step needs facts, not code: the owner's screen mode (depth / RTG), and ideally a rig or the owner's machine
replaying the same two click byte strings (they are now C literals in the test) with the real chat.png.
