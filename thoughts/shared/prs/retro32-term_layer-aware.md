---
date: 2026-09-28
topic: "retro32-term: draw through the window when covered, RTG, 16-pixel cells"
tags: [retro32-term, dctelnet, builtin-renderer, pr]
status: draft
---

For bruno-frederic/retro32-term. Branch `feat/layer-aware` (three commits on
b291015). Not pushed yet: it needs a fork or push access.

## Summary

The engine drew straight into the screen bitmap everywhere. That is the reason
for its speed. But on a screen shared with windows it drew over them,
scrolled their pixels along with the text, and left behind what a closed
window uncovered (in DCTelnet: half a "Connecting..." window on the terminal).
It also could not run on RTG screens ("Failed to setup retro32-term, error # 1").

It now has two ways to draw:

- **Directly into the bitplanes**, unchanged. It is used while nothing covers
  the terminal, on a 4-plane screen whose pens 0-15 are the ANSI colours.
- **Through the terminal window's RastPort** in every other case:
  - while something covers the terminal (the caller reports it with
    `term_covered()`);
  - on a bitmap that is not planar (RTG);
  - on a deeper screen, where the upper planes' bits showed through as a
    grey background;
  - with other pens (`term_set_pens()`: the Workbench's shared pens).

  It uses `Text()` in runs of equal pens, `RectFill()`, `ScrollRaster()`
  (plus an explicit fill of the uncovered strip: V39 leaves it to a backfill
  hook) and a COMPLEMENT fill for the cursor. The window's smart-refresh layer
  keeps what is covered and gives it back.

The terminal also lies in a rectangle of the bitmap (`term_init_area()`): the
text area of its window. So it starts below a screen title bar and has the
window's rows, not the screen's. And cells are as wide as the font
(`term_cols`, `term_set_font()`): the C64's 16x8 font gives 40 columns.

`term_init(screen, font)` keeps its meaning (whole screen, direct only), so
`retro32term.c` builds unchanged.

## Speed

Measured on cycle-exact FS-UAE with a harness that feeds the engine a fixed
workload:

- "scroll": 300 coloured lines;
- "art": 12 cursor-addressed full-screen colour art screens.

Times in ms, DateStamp timing, repeatable within ±80 ms:

| | A500 scroll | A500 art | A1200 scroll | A1200 art |
|---|---|---|---|---|
| before | 43 360 | 23 940 | 36 620 | 5 800 |
| after, direct | 43 640 | 23 940 | 36 600 | 5 800 |
| after, RastPort path | 50 940 | 28 840 | 37 840 | 5 140 |

The direct path pays one test per glyph (`term_alt`, shared by the RastPort
path and 16-pixel cells). The RastPort path runs only while something covers
the terminal, or where direct drawing cannot be right.

A cell buffer was tried first and dropped. The engine would have kept its own
copy of the screen and redrawn from it. It cost +3 to 8 % on the A500 always
(every store competes with the hires bitplane DMA), and a permanent window
would have kept it in the slow redraw.

## Testing

- DCTelnet's `tests/test_term_engine.c` runs the engine on the host with the
  graphics.library calls recorded. It checks that:
  - while covered, on RTG, on 8 planes and with a pen map, nothing is
    written into the planes;
  - the Text/ScrollRaster/COMPLEMENT calls have the window's coordinates and
    the mapped pens;
  - uncovered, it draws directly again, in its own area only (the title bar
    is left alone);
  - 16-pixel cells give 40 columns, two bytes per row, and wrap at 40.

  A mutant with `term_covered()` doing nothing fails it.
- DCTelnet on the emulated A1200:
  - a window over the streaming terminal stays whole, and closing it
    mid-stream leaves no trace;
  - the 24-bit RTG Workbench and an AGA 256-colour screen show a black
    background and the ANSI colours;
  - a PETSCII session shows the C64 font at 40 columns.

🤖 Generated with [Claude Code](https://claude.com/claude-code)
