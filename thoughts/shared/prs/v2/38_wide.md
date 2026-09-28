---
date: 2026-09-28
topic: "PR #38: 132 Columns"
tags: [dctelnet, pr, v2.0-dev]
status: final
---

# #38 132 Columns

Head `feature/pr-wide` on spotUP/dctelnet, base v2.0-dev.

**Based on v2.0-dev, stacked on #37.** Only the last commit belongs here.

This PR was first opened against `main`; it now carries the same feature ported onto v2.0-dev (its Prefs format, State bits and menus).

## Summary

Display > 132 Columns makes the terminal 132 columns wide instead of 80
(Unix hosts, wide BBS menus). The Workbench window widens as far as the
screen allows; on DCTelnet's own screen the terminal is centred at the
new width. The BBS is told the grid through NAWS.

The item needs the ibmcon.device renderer (grids up to 199 columns): it
is disabled under the other renderers, and ArtColumns() stays at 80
there even when the setting is on. The setting is State bit 18
(APP_132_COLUMNS); an old DCTelnet.Prefs keeps it (old flag bit 19).

## Testing on v2.0-dev

- Builds with vbcc for 68000 and 68020, warnings as errors; `cd tests && make` passes.
- Emulated A1200 (FS-UAE, driven through amiagent): Display > 132 Columns: the Workbench window widens to the screen (NAWS 97 on an 800-pixel Workbench), and back to 80.

<details><summary>Testing of the version for main (the same feature)</summary>

- Host tests: `cd test && make` (`test_screenfont`: a window 132 cells wide gives a 132-column grid).
- Emulated A1200, Workbench 800 pixels wide:
  - NAWS 80x25 at connect;
  - after the menu item 97x25, because the window stopped at the screen edge;
  - after switching it off, 80x25 again.

</details>

Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01DmFJdpcHF7rhRU6XnPvz6P
