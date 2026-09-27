**Stacked on #17.** Only the commits after it belong here.

## Summary

The terminal's size, as the BBS sees it and as the window shows it:

- **Topaz Pro**: topaz was drawn for the Amiga's tall pixels; on square-pixel modes (every RTG mode) it is replaced by Topaz Pro 8x16, a redraw for square pixels (bundled in `Fonts/`). topaz and Topaz Pro are treated as one face, so a setting naming either opens the right one for the mode.
- **Window size to the BBS (NAWS)** was the outer window size (title bar and borders included): a 640x200 Workbench window was reported as 80x12 instead of 77x11. It now reports the text area, and again after every resize.
- **80 columns**: BBS art is drawn for 80 columns and wraps at that edge. The Workbench window stops at 80 columns (40 in PETSCII Mode); on a wider own screen the terminal is 80 columns wide and centred, with black around it.
- **80x25**: the Workbench window opens 80x25 characters of its font on any mode unless Snapshot Windows saved a size.
- An entry's own font or palette on the Workbench reopens only the console, not the window.

## Testing

- Host unit tests (`ScreenFont_Grid`, width limit, font choice per pixel shape): `cd test && make`.
- Tested on an emulated A1200 with a local telnet server that records NAWS: 80x25 / 77x11 / 80x47 as expected.

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01DmFJdpcHF7rhRU6XnPvz6P
