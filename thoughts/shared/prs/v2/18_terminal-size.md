---
date: 2026-09-28
topic: "PR #18: Terminal size: Topaz Pro, 80 columns, window size to the BBS"
tags: [dctelnet, pr, v2.0-dev]
status: final
---

# #18 Terminal size: Topaz Pro, 80 columns, window size to the BBS

Head `feature/pr-terminal-size` on spotUP/dctelnet, base v2.0-dev.

**Based on v2.0-dev, stacked on #17.** Only the last commit belongs here.

This PR was first opened against `main`; it now carries the same feature ported onto v2.0-dev (its Prefs format, State bits and menus).

## Summary

The terminal's size, as the BBS sees it and as the window shows it:
- Topaz Pro: topaz was drawn for the Amiga's tall pixels; on
  square-pixel modes (every RTG mode) it is replaced by Topaz Pro 8x16,
  a redraw for square pixels (build/package/DCTelnet/Fonts). topaz and
  Topaz Pro are one face: a setting naming either opens the right one
  for the mode. The built-in renderer keeps its 8x8 font.
- Window size to the BBS (NAWS) was the outer window size (title bar
  and borders included); it is now the text area, and again after every
  resize.
- 80 columns: BBS art is drawn for 80 columns and wraps at that edge.
  The Workbench window stops at 80 columns (40 in PETSCII Mode); on a
  wider own screen the terminal is 80 columns wide and centred, with
  black around it.
- 80x25: the Workbench window opens 80x25 characters of its font on any
  mode unless Snapshot Windows saved a size (APP_WINDOW_SNAPSHOT).
- An entry's own font or palette on the Workbench reopens only the
  console, not the window.

Tests: tests/test_screenfont.c (grid, width limit, font per pixel shape);
the font check covers TopazPro.font.

## Testing on v2.0-dev

- Builds with vbcc for 68000 and 68020, warnings as errors; `cd tests && make` passes.
- Emulated A1200 (FS-UAE, driven through amiagent): 132 and 80 columns reported through NAWS as the window allows (see #38).

<details><summary>Testing of the version for main (the same feature)</summary>

- Host unit tests (`ScreenFont_Grid`, width limit, font choice per pixel shape): `cd test && make`.
- Tested on an emulated A1200 with a local telnet server that records NAWS: 80x25 / 77x11 / 80x47 as expected.

</details>

Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01DmFJdpcHF7rhRU6XnPvz6P
