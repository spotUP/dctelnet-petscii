---
date: 2026-09-28
topic: "PR #19: Newest ibmcon and libraries from DCTelnet's drawer; start-up screen"
tags: [dctelnet, pr, v2.0-dev]
status: final
---

# #19 Newest ibmcon and libraries from DCTelnet's drawer; start-up screen

Head `feature/pr-drawer-components` on spotUP/dctelnet, base v2.0-dev.

**Based on v2.0-dev, stacked on #18.** Only the last commit belongs here.

This PR was first opened against `main`; it now carries the same feature ported onto v2.0-dev (its Prefs format, State bits and menus).

## Summary

- Newest copy of ibmcon and the libraries. DCTelnet ships
  Devs/ibmcon.device and Libs/ (reqtools, XEM, XPR) in its own drawer
  but only opened DEVS:/LIBS:, so an older ibmcon installed there was
  used instead. OpenNewestDevice()/OpenNewestLibrary() compare the $VER
  of both copies and open the newer (DCTelnet's on a tie); an older copy
  already in memory is expunged first when nothing uses it. Fonts are
  tried from DCTelnet's Fonts drawer first.
- Start-up screen: one labelled line per fact (Renderer, Screen, Text
  area), none wider than 80 columns; it says where ibmcon came from.
- Font fallback: a font that cannot be opened falls back per pixel shape
  and the start-up screen names it; Screen Font switches in place on the
  Workbench.
- Title bar: windows and the title-bar LEDs are placed by the title bar
  the screen actually draws, not by the font size setting (with Topaz Pro
  16 the first rows went under the bar).

Tests: tests/test_progdir.c (ProgDir_Path, $VER parsing and comparison).

## Testing on v2.0-dev

- Builds with vbcc for 68000 and 68020, warnings as errors; `cd tests && make` passes.

<details><summary>Testing of the version for main (the same feature)</summary>

- Host unit tests (`ProgDir_Path`, `$VER` parsing and comparison): `cd test && make`.
- Tested on an emulated A1200, including an older ibmcon left in memory by another program: DCTelnet expunged it and opened its own newer copy.

</details>

Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01DmFJdpcHF7rhRU6XnPvz6P
