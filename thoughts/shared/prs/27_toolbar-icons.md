---
date: 2026-09-28
topic: "PR #27 description -- toolbar-icons"
tags: [dctelnet, pr]
status: final
---

**Stacked on #26.** Only the last two commits belong here.

## Summary

The tool bar icons from the 1.6 package were planar images for fixed pens: 8 colours of the MagicWB palette. On a standard Workbench palette they came out pink and cyan, on DCTelnet's own 256-colour screen cyan and blue.

- **Icons carry their colours.** Each icon names the colour of its pens in the tool type `DCTELNET_PALETTE=RRGGBB,...`. `OpenToolBarWindow()` redraws the images with the screen's closest shared pens (`ObtainBestPen`, `FindColor` when none is free; pen 0 is the window background). On OS 2 the colours are matched to the 4 standard Workbench colours. An icon without the tool type, such as one a user drew, is drawn as it is. The matching and the planar remap are in `src/iconpens.c` (pure, host-tested).
- **New flat, antialiased icons** from Lucide (ISC, licence in `Docs/Lucide-LICENSE.txt`): black on grey, white on blue pressed, 8 colours. `tools/toolbar_icons.py` renders them from the SVGs in `tools/toolbar_icons/` and checks them (`make check-icons` in `test/`).
- **Two sets by pixel shape**, not by Workbench or own screen: `ToolBar/Square` (24x24 symbols) and `ToolBar/Wide` (48x24) for modes whose pixels are twice as tall, such as 640x256, where square art looked squeezed. `ToolBar/` replaces `WBIcons` and `SCIcons`; the installer copies it.
- **Own screen:** the ANSI pens and the AGA pointer pens 16-19 are now obtained exclusively, so no shared-pen request (the icons') can recolour them.
- `ObtainNearestPen()` (utils.c) is shared by the icons and the Workbench ANSI pens, which had the same code inline.

## Testing

- Host tests: `cd test && make` (`test_iconpens`, `test_screenfont`, `check-icons`; the icon check fails on the old icons).
- Emulated A1200: Picasso96 24-bit Workbench (square set) and AGA 640x256 256 colours on the own screen (wide set), tool bar opened at start and through Options > Tool Bar.

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01DmFJdpcHF7rhRU6XnPvz6P
