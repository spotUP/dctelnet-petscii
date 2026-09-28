---
date: 2026-09-28
topic: "PR #30 description -- ibmcon-1.11"
tags: [dctelnet, pr]
status: final
---

**Stacked on #29.** Only the last commit belongs here.

## Summary

ibmcon.device **1.11**. ibmcon drew straight into the window's RastPort and kept nothing, so no client could read the screen back (to copy text to the clipboard or save the screen) and no cell could be redrawn (blink).

- **Screen buffer.** Every cell: character, fg and bg pen, attribute flags (4 bytes a cell). It is sized in `MeasureGrid` and keeps what overlaps when the grid changes. Text, the erase commands (`J`, `K`, `X`), the scrolls (`S`, `T`, `L`, `M`, line feed and cursor-up scrolling at the margins) and `@`/`P` all mirror themselves in it. Without memory for it the device works as before.
- **`IBMCMD_READTEXT` ($7FE3)** copies row `io_Offset` (1-based) to `io_Data`, at most `io_Length` bytes; `io_Actual` = bytes (0 outside the screen); `IOERR_NOCMD` without a buffer.
- **Blink.** SGR 5 cells blink every half second: a `timer.device` request in the handler, the cells redrawn from the buffer with the cursor hidden meanwhile. In iCE mode (`CSI ?33h`, #28) blink stays a bright background. Nothing is redrawn while no cell blinks.
- The handler's frame grows by $18 bytes for the new fields.

## Testing

- `ibmcon/tests/readtext_probe.c` opens the device on a window, sends each case and reads the rows back through `IBMCMD_READTEXT`: X, P, @, M, L, K, 1K, line feed at the bottom, S, T, cursor up at the top, 2J, and the attributes of a bold red on blue character. 17 of 17 pass on an emulated A1200.
- Blink: screenshots 0.2 s apart alternate between the red text and blank background; the steady text beside it never changes.
- `make` in `ibmcon/` (68000 check).

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01DmFJdpcHF7rhRU6XnPvz6P
