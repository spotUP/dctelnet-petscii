---
date: 2026-09-28
topic: "PR #17: 256-colour AGA and RTG screens, ANSI colours on their own pens"
tags: [dctelnet, pr, v2.0-dev]
status: final
---

# #17 256-colour AGA and RTG screens, ANSI colours on their own pens

Head `feature/pr-rtg-colours` on spotUP/dctelnet, base v2.0-dev.

**Based on v2.0-dev, stacked on #16.** Only the last commit belongs here.

This PR was first opened against `main`; it now carries the same feature ported onto v2.0-dev (its Prefs format, State bits and menus).

## Summary

With the ibmcon.device renderer:
- On a 32+ colour own screen (AGA, RTG) the 16 ANSI colours get pens of
  their own (8-15, 20-27), handed to ibmcon with IBMCMD_SETPENS (1.5);
  the UI keeps Intuition's pens and colours. They are loaded in ANSI
  order from prefs.DeviceColors, the device palette, so one palette
  serves every depth; the palette editor edits them there.
- On the Workbench the ANSI colours take 16 shared pens of exactly their
  colours (ObtainBestPen, V39), released on close; a colour the
  Workbench cannot share uses the nearest one, which is not released.
- The start-up block says which ibmcon runs, its depth and whether the
  ANSI colours are on their own pens (an ibmcon older than 1.5 cannot).
For every renderer:
- Screen modes up to 256 colours (ReqTools) and RTG depths (ASL).
- The depth of RTG screens is read with GetBitMapAttr (also for XEM).
- Exposed window areas are terminal background (a backfill hook), not
  grey pen 0; the overscan border stays black (border blank, V39).
- The LEDs and the Quit item use the right pen for the palette in use.
- The palette can only be edited on DCTelnet's own screen.

Also: ibmcon/ holds the console's source (1.4 bug-fixed to 1.6), and
tools/rtgprobe measures depth queries and COMPLEMENT on real screen
modes.

Tests: tests/test_palette.c (pen layout, device palette <-> ANSI order).

## Testing on v2.0-dev

- Builds with vbcc for 68000 and 68020, warnings as errors; `cd tests && make` passes.
- Emulated A1200 (FS-UAE, driven through amiagent): The own screen at 256 colours with ibmcon: black background, ANSI colours on their own pens.

<details><summary>Testing of the version for main (the same feature)</summary>

- Host unit tests (palette conversion, pen layout): `cd test && make`.
- `tools/rtgprobe` measures depth queries and COMPLEMENT on real screen modes (used to find that an RTG bitmap's `bm_Depth` is not its depth).
- Tested on an emulated A1200: AGA 256 colours, Picasso96 8/16/24-bit, 16-colour and the Workbench.

</details>

Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01DmFJdpcHF7rhRU6XnPvz6P
