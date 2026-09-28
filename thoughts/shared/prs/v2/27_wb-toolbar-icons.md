---
date: 2026-09-28
topic: "PR #27: Tool bar icons that suit every screen"
tags: [dctelnet, pr, v2.0-dev]
status: final
---

# #27 Tool bar icons that suit every screen

Head `feature/pr-wb-toolbar-icons` on spotUP/dctelnet, base v2.0-dev.

**Based on v2.0-dev, stacked on #26.** Only the last commit belongs here.

This PR was first opened against `main`; it now carries the same feature ported onto v2.0-dev (its Prefs format, State bits and menus).

## Summary

The tool bar icons were MagicWB icons (8 fixed pens of the MagicWB
palette): on any other palette -- a standard Workbench, DCTelnet's own
screen -- they came out in wrong colours.
- New icons (Lucide symbols, ISC licence) that carry their colours: an
  8-colour image plus a DCTELNET_PALETTE tool type; DCTelnet maps each
  colour to the nearest pen of the screen the tool bar opens on
  (src/iconpens.c, ObtainNearestPen in utils.c), so they suit every
  palette and can be antialiased. Black on grey, white on blue pressed.
- ToolBar/Square and ToolBar/Wide: the same symbols drawn for square
  pixels and for the Amiga's tall ones (hires without interlace); the
  old WBIcons and SCIcons drawers are gone.
- On DCTelnet's own screen the icon pens never take the ANSI colours' or
  the pointer's pens.
- tools/toolbar_icons.py builds the icons from the SVGs and checks them
  (part of tests/).

## Testing on v2.0-dev

- Builds with vbcc for 68000 and 68020, warnings as errors; `cd tests && make` passes.

<details><summary>Testing of the version for main (the same feature)</summary>

- Host tests: `cd test && make` (`test_iconpens`, `test_screenfont`, `check-icons`; the icon check fails on the old icons).
- Emulated A1200: Picasso96 24-bit Workbench (square set) and AGA 640x256 256 colours on the own screen (wide set), tool bar opened at start and through Options > Tool Bar.

</details>

Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01DmFJdpcHF7rhRU6XnPvz6P
