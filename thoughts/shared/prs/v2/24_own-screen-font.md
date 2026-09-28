---
date: 2026-09-28
topic: "PR #24: A font or palette change on the own screen reopens only the terminal"
tags: [dctelnet, pr, v2.0-dev]
status: final
---

# #24 A font or palette change on the own screen reopens only the terminal

Head `feature/pr-own-screen-font` on spotUP/dctelnet, base v2.0-dev.

**Based on v2.0-dev, stacked on #23.** Only the last commit belongs here.

This PR was first opened against `main`; it now carries the same feature ported onto v2.0-dev (its Prefs format, State bits and menus).

## Summary

path; the built-in and XEM renderers reopen the display as before.
Changing the font (Settings > Screen Font, or an Address Book entry's
Screen group) on DCTelnet's own screen closed and reopened the whole
screen; on the Workbench only the console was reopened. The own screen
now takes the same path: ReopenTerminal() reopens the console with the new
font in the same window on the same screen, loads the settings' palette
onto the terminal's pens, and the 80-column centring follows the new
cell width. The screen's title bar and menus keep the screen's font until
the screen is next opened.

The BBS is told the new grid (NAWS) after every terminal reopen: on the
own screen no window resize would send it.

Verified on the emulated A1200, AGA 256 colours: the screen stays the same
(same address), the terminal switches to Thin711 7x11, and the BBS
receives NAWS 80x30 then 80x44.

## Testing on v2.0-dev

- Builds with vbcc for 68000 and 68020, warnings as errors; `cd tests && make` passes.

<details><summary>Testing of the version for main (the same feature)</summary>

On an emulated A1200 (AGA, 256 colours): the screen stays the same (same address), the terminal switches to a 7x11 font, and a local telnet server receives NAWS 80x30, then 80x44.

</details>

Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01DmFJdpcHF7rhRU6XnPvz6P
