---
date: 2026-09-28
topic: "PR #31: Select text with the mouse, Edit > Paste, Copy Screen"
tags: [dctelnet, pr, v2.0-dev]
status: final
---

# #31 Select text with the mouse, Edit > Paste, Copy Screen

Head `feature/pr-clipboard` on spotUP/dctelnet, base v2.0-dev.

**Based on v2.0-dev, stacked on #30.** Only the last commit belongs here.

This PR was first opened against `main`; it now carries the same feature ported onto v2.0-dev (its Prefs format, State bits and menus).

## Summary

- Dragging the left mouse button over the terminal selects text: shown
  inverted, copied to the clipboard (FTXT/CHRS, clipboard unit 0) when
  the button goes up, and shown until the next click or text. The text
  is read back from ibmcon's screen buffer (IBMCMD_READTEXT, ibmcon
  1.11): with the ibmcon.device renderer only; the other renderers keep
  no text to read, and Copy Screen is disabled there.
- A new Edit menu: Paste (Right-Amiga+V) types the clipboard's text
  (Return rules, telnet IAC doubling, LF as CR); Copy Screen copies the
  whole screen.
- src/clip.c (host-tested) turns screen rows into clipboard text (blank
  tails trimmed, CP437 glyphs as their Amiga look-alikes, src/charset.c)
  and pasted text into bytes for the BBS.

## Testing on v2.0-dev

- Builds with vbcc for 68000 and 68020, warnings as errors; `cd tests && make` passes.
- Emulated A1200 (FS-UAE, driven through amiagent): Select with the mouse, Edit > Copy (Amiga-C), then Edit > Paste into a logger connection: the server received the selected text.

<details><summary>Testing of the version for main (the same feature)</summary>

- Host tests: `cd test && make` (`test_clip`: FTXT build and parse with other chunks skipped, selection spans, trailing blanks, paste bytes, CP437 and PETSCII conversion).
- Emulated A1200, events injected through input.device: a drag from row 1 column 1 to row 2 column 20, then Right-Amiga+V. The local server received `Hello World\rsecond line -- end` (CP437 `C4 C4` as `--`), and the selection stayed inverted.

</details>

Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01DmFJdpcHF7rhRU6XnPvz6P
