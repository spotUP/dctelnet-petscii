---
date: 2026-09-28
topic: "PR #30: feat(ibmcon): 1.11 -- screen buffer, IBMCMD_READTEXT, blink"
tags: [dctelnet, pr, v2.0-dev]
status: final
---

# #30 feat(ibmcon): 1.11 -- screen buffer, IBMCMD_READTEXT, blink

Head `feature/pr-ibmcon-1.11` on spotUP/dctelnet, base v2.0-dev.

**Based on v2.0-dev, stacked on #29.** Only the last commit belongs here.

This PR was first opened against `main`; it now carries the same feature ported onto v2.0-dev (its Prefs format, State bits and menus).

## Summary

ibmcon drew straight into the RastPort and kept nothing: no client could
read the screen back (copy to the clipboard, save the screen) and no
cell could be redrawn (blink).

- A buffer of every cell (character, fg, bg, attribute flags; 4 bytes a
  cell), sized in MeasureGrid and kept where a resized grid overlaps.
  Text, J/K/X, S/T/L/M, line feed and cursor-up scrolling at the margins
  and @/P mirror themselves in it. Without memory: as before.
- IBMCMD_READTEXT ($7FE3) copies row io_Offset to io_Data.
- SGR 5 cells blink every half second (timer.device in the handler,
  redrawn from the buffer, cursor hidden meanwhile); in iCE mode blink
  stays a bright background; no redraws while nothing blinks.
- The handler frame grows by $18 bytes for the new fields.

tests/readtext_probe.c checks every drawing path on an Amiga through
IBMCMD_READTEXT: 17 of 17 pass on an emulated A1200. Blink checked with
screenshots 0.2 s apart: the blink cells alternate, the text beside them
does not change.

## Testing on v2.0-dev

- Builds with vbcc for 68000 and 68020, warnings as errors; `cd tests && make` passes.

<details><summary>Testing of the version for main (the same feature)</summary>

- `ibmcon/tests/readtext_probe.c` opens the device on a window, sends each case and reads the rows back through `IBMCMD_READTEXT`: X, P, @, M, L, K, 1K, line feed at the bottom, S, T, cursor up at the top, 2J, and the attributes of a bold red on blue character. 17 of 17 pass on an emulated A1200.
- Blink: screenshots 0.2 s apart alternate between the red text and blank background; the steady text beside it never changes.
- `make` in `ibmcon/` (68000 check).

</details>

Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01DmFJdpcHF7rhRU6XnPvz6P
