---
date: 2026-09-28
topic: "PR #38 description -- wide"
tags: [dctelnet, pr]
status: final
---

**Stacked on #37.** Only the last commit belongs here.

## Summary

**Options > 132 Columns** makes the terminal 132 columns wide instead of 80, for Unix hosts and wide BBS menus. It goes through `ArtColumns()`, which every width rule uses; PETSCII stays at 40.

When it is switched while running:

- the Workbench window widens or narrows, at most to the screen's width;
- on the own screen the terminal is centred;
- the BBS is told the grid it actually got. On the Workbench the window's resize sends NAWS; on the own screen, where no resize message comes, it is sent directly.

## Testing

- Host tests: `cd test && make` (`test_screenfont`: a window 132 cells wide gives a 132-column grid).
- Emulated A1200, Workbench 800 pixels wide:
  - NAWS 80x25 at connect;
  - after the menu item 97x25, because the window stopped at the screen edge;
  - after switching it off, 80x25 again.

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01DmFJdpcHF7rhRU6XnPvz6P
