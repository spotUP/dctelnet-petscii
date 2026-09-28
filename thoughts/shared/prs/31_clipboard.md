---
date: 2026-09-28
topic: "PR #31 description -- clipboard"
tags: [dctelnet, pr]
status: final
---

**Stacked on #30** (it reads the screen through ibmcon 1.11's `IBMCMD_READTEXT`). Only the last commit belongs here.

## Summary

DCTelnet had no way to get text out of the terminal or to paste text in.

- **Select with the mouse.** Dragging with the left button over the terminal selects text in reading order, shown inverted; letting go copies it to the clipboard (IFF FTXT). The selection stays until the next click or until text arrives. The inversion is drawn on a copy of the window's RastPort, because ibmcon's blink timer draws on the window's own from its task.
- **The text** is read back from ibmcon's screen buffer. CP437 becomes ISO-8859-1 where Latin-1 has the character, line drawing the closest ASCII (`+ - | #`); in PETSCII Mode the C64 characters in the case set on screen.
- **New Edit menu:** Paste (Right-Amiga+V) types the clipboard's text like typed keys: line ends as Return (CR, or CR LF with Return = CR + LF), a 255 byte doubled on telnet. Copy Screen copies the whole screen.
- With an XEM library or an ibmcon older than 1.11 there is nothing to read back and selecting does nothing.
- Fix: `GetNewMenuItemFromID()` looped `sizeof(mainMenuDesc)` times, which counts bytes, not entries, so an ID not in the table read past it.

Right-Amiga+C is kept for Reset Screen; copying needs no shortcut, since letting go of the mouse button copies.

## Testing

- Host tests: `cd test && make` (`test_clip`: FTXT build and parse with other chunks skipped, selection spans, trailing blanks, paste bytes, CP437 and PETSCII conversion).
- Emulated A1200, events injected through input.device: a drag from row 1 column 1 to row 2 column 20, then Right-Amiga+V. The local server received `Hello World\rsecond line -- end` (CP437 `C4 C4` as `--`), and the selection stayed inverted.

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01DmFJdpcHF7rhRU6XnPvz6P
