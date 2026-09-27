**Stacked on #20.** Only the commits after it belong here.

## Summary

- **An editor for the 16 ANSI colours** (Settings > Screen Palette, now also on the Workbench): the colours in ANSI order with their names, Red/Green/Blue sliders, Use / Default / Cancel. On DCTelnet's own screen it edits the terminal's pens directly; on the Workbench it borrows exclusive pens for the preview and hands the console new shared pens on Use without clearing it. Only the pens it borrowed are given back.
- **True-colour screens**: recolouring a pen does not change pixels already drawn, so the editor redraws its preview and rewrites the terminal's pixels of the edited colour (cybergraphics `ReadPixelArray`/`WritePixelArray`), never merging two colours.
- **Menus on 32+ colours**: menu items were created without a front pen and used pen 0, which is the UI's grey there; every item looked disabled. They now use the screen's menu text pen.

## Testing

- Host unit tests (default palette, channels, safe recolouring): `cd test && make`.
- Tested on an emulated A1200 (Picasso96 24-bit Workbench, AGA 256 colours).

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01DmFJdpcHF7rhRU6XnPvz6P
