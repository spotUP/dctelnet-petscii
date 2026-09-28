---
date: 2026-09-28
topic: "PR #21: An editor for the 16 ANSI colours; menu items in the menu text pen"
tags: [dctelnet, pr, v2.0-dev]
status: final
---

# #21 An editor for the 16 ANSI colours; menu items in the menu text pen

Head `feature/pr-palette-editor` on spotUP/dctelnet, base v2.0-dev.

**Based on v2.0-dev, stacked on #20.** Only the last commit belongs here.

This PR was first opened against `main`; it now carries the same feature ported onto v2.0-dev (its Prefs format, State bits and menus).

## Summary

Screen Palette opens an editor for the 16 ANSI colours instead of the
ReqTools palette requester: a colour picker with exactly the 16 colours
in ANSI order, the colour's name, and Red/Green/Blue sliders (0-15, the
RGB4 the settings keep), with Use, Default and Cancel.
- It edits the palette the renderer shows: AnsiColors for the built-in
  and XEM renderers, DeviceColors for the console devices (shown in ANSI
  order either way); Default restores that renderer's default palette.
  Also for an Address Book entry's own palette (the settings window).
- On DCTelnet's own screen the terminal recolours while the sliders
  move; on an RTG true-colour screen its pixels are recoloured
  (cybergraphics), unless two ANSI colours share a value.
- On the Workbench with ibmcon (OS 3.0) it previews on 16 borrowed pens,
  gives back only those, and the running console takes new shared pens.
- Menu items are drawn in the screen's menu text pen: with pen 0 every
  item looked disabled on 32+ colours.

The default palettes moved from prefs.c to palette.c, next to the code
that converts them; tests: tests/test_palette.c (the renderer's palette
in ANSI order, channels, recolouring).

## Testing on v2.0-dev

- Builds with vbcc for 68000 and 68020, warnings as errors; `cd tests && make` passes.

<details><summary>Testing of the version for main (the same feature)</summary>

- Host unit tests (default palette, channels, safe recolouring): `cd test && make`.
- Tested on an emulated A1200 (Picasso96 24-bit Workbench, AGA 256 colours).

</details>

Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01DmFJdpcHF7rhRU6XnPvz6P
