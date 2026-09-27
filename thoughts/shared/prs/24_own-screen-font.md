**Stacked on #23.** Only the last commit belongs here.

## Summary

Changing the font (Settings > Screen Font, or an Address Book entry's Screen group) on DCTelnet's **own screen** closed and reopened the whole screen, while on the Workbench only the console was reopened (#18). The own screen now takes the same in-place path:

- The console reopens with the new font in the same window on the same screen; the settings' palette is loaded onto the terminal's pens; the 80-column centring follows the new cell width.
- The screen's title bar and menus keep the screen's font until the screen is next opened.
- The BBS is told the new grid (NAWS) after every terminal reopen; on the own screen no window resize would send it.

## Testing

On an emulated A1200 (AGA, 256 colours): the screen stays the same (same address), the terminal switches to a 7x11 font, and a local telnet server receives NAWS 80x30, then 80x44.

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01DmFJdpcHF7rhRU6XnPvz6P
