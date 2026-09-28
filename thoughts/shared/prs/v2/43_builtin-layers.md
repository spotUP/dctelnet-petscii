---
date: 2026-09-28
topic: "PR #43: Built-in renderer: never over windows; RTG and 256 colours; PETSCII in the C64 font"
tags: [dctelnet, pr, v2.0-dev]
status: final
---

# #43 Built-in renderer: never over windows; RTG and 256 colours; PETSCII in the C64 font

Head `feature/pr-builtin-layers` on spotUP/dctelnet, base v2.0-dev.

**Based on v2.0-dev, stacked on #42.** Only the last 4 commits belong here.

The engine changes are in the retro32-term submodule (branch `feat/layer-aware` of spotUP/retro32-term), offered upstream as https://github.com/bruno-frederic/retro32-term/pull/1; this PR moves the submodule to that branch.

## Summary

### Built-in renderer: never draw over windows on its screen

retro32-term drew straight into the screen bitmap, also under DCTelnet's
own windows over the terminal (the scrollback window, requesters, the
"Connecting..." window) and open menus: it drew over them, scrolled them
along with the text, and left what a closed window had covered as it was.
It also drew its top rows under the screen's title bar, and had 32 rows
where the window (and the BBS) had 30.

- The terminal is the text area of the terminal window (BuiltinInit ->
  term_init_area): below the title bar, the window's rows.
- Before each write and on each pass of the main loop, BuiltinCovered()
  looks for a layer in front of the terminal window that overlaps it;
  while there is one, retro32-term draws through the window's RastPort
  (layer-aware) instead of into the bitplanes (retro32-term submodule,
  branch feat/layer-aware). Glyphs are collected into runs; term_flush()
  draws the last one before the cursor is shown.

tests/test_term_engine.c runs the engine on the host with the
graphics.library calls recorded: directly into its own area when
uncovered, never into the planes when covered or on a bitmap that is not
planar, and the Text/ScrollRaster/COMPLEMENT calls with the window's
coordinates and the pen map.

Checked on the emulated A1200: the scrollback window stays whole while
text streams beneath it, and closing it mid-stream leaves the right text,
no remains of the window. Speed, cycle-exact A500/A1200: unchanged.

### Built-in renderer on RTG and 256-colour screens

retro32-term drew only into the 4 bitplanes of a 16-colour screen: on an
RTG screen it failed ("Failed to setup retro32-term, error # 1",
console.device instead), on an AGA 256-colour screen it showed a grey
background and wrong colours. It now draws through the terminal
window's RastPort wherever direct drawing cannot be right (retro32-term
submodule, feat/layer-aware):

- On the Workbench (RTG or not) the ANSI colours take 16 shared pens of
  exactly the palette's colours, as ibmcon's do (ObtainWorkbenchPens,
  now for both renderers, from Palette_Get), handed to the engine; a
  palette edit renews them.
- Any screen mode of at least 640x200 with 16 colours or more suits the
  built-in renderer (Prefs_ScreenFits); switching to it asks for a mode
  only when the current one is smaller. A 4-plane screen keeps the fast
  direct drawing. Prefs_MaxDepth went: every renderer takes up to 32
  bitplanes.

Checked on the emulated A1200: a Workbench window on the 24-bit RTG
Workbench, and the own screen at 256 colours, both with a black
background and the ANSI colours, scrolling cleanly.

### Built-in renderer: PETSCII sessions in the C64 font

A PETSCII session on the built-in renderer showed the BBS in CP437
lookalikes, 80 columns wide. It now opens the C64 fonts as the console
devices do (Petscii.font, PetsciiLower.font: 16x8 cells, 40 columns),
takes the raw-glyph stream, and switches the engine's font on the
charset codes (retro32-term: cells as wide as the font).

Two faults on the way, both older than this:

- Switching to the C64 display is a full reopen for the built-in and
  XEM renderers, and the main loop ran the pending connect before it:
  the BBS's first screen was drawn on the old display, then cleared (a
  blank screen). The in-place reopen runs first now, then the full one
  it may ask for, then the connect.
- A packet with a charset switch in it was translated whole and drawn
  in the charset it ended with -- also on ibmcon. It is cut after each
  switch (petscii_part_length, host-tested): each part in the font it
  was sent for.

Checked on the emulated A1200: C64 glyphs, colours, reverse video,
graphics characters, the wrap at 40 columns, and upper-case text before
a switch to the lower-case set in the same packet.

### Settings converted from 1.x get colours for the built-in renderer

A DCTelnet 1.x settings file has one palette, the console's (ibmcon
order). Converted, it filled DeviceColors and left AnsiColors, the
built-in and XEM renderers' palette, all zero: an Address Book entry
from 1.x showed black text on black with the built-in renderer, on the
own screen and on the Workbench (its shared pens were black too). The
global settings escaped it only because their palette was validated;
an entry's never was.

Palette_Repair() (palette.c) makes both palettes usable: one that is
not (all zero, or not RGB4) becomes the other in its colour order, or
the default. Prefs_FromLegacy(), SitePrefs_Decode() and the global
validation all use it, so an entry already saved with a black palette
heals on load.

## Testing on v2.0-dev

- Builds with vbcc for 68000 and 68020, warnings as errors; `cd tests && make` passes.
- Emulated A1200 (FS-UAE, driven through amiagent): The own 4-plane screen: a scroll back window over the streaming terminal stays whole and leaves nothing when closed mid-stream; the 24-bit RTG Workbench and a 256-colour own screen: black background and the ANSI colours; a PETSCII board: C64 font, 40 columns, colours, reverse, graphics characters, both charsets.

Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01DmFJdpcHF7rhRU6XnPvz6P
