---
date: 2026-09-28
topic: "PR #28 description -- ibmcon-1.10"
tags: [dctelnet, pr]
status: final
---

**Stacked on #27.** Only the last commit belongs here.

## Summary

ibmcon.device **1.10**: console sequences today's BBSes send that ibmcon ignored or got wrong.

- **iCE colours.** `CSI ?33h` switches iCE mode (as in SyncTERM): SGR 5 then gives a bright background (pens 8-15), which ANSI art drawn for 16 background colours relies on. SGR 25 ends it. Without iCE mode SGR 5 still draws nothing.
- **`CSI X` (erase characters).** Used by full-screen editors. The CSI table is full, so it takes the slot of the `CSI R` stub (a report a terminal sends, never receives). The assembler now fails if the table grows past `$DA` or the `$VER` string moves it.
- **`ESC 7` / `ESC 8`** save and restore the cursor, as `CSI s` / `CSI u`. The restored position is clamped to a grid that shrank since.
- **DECCKM (`CSI ?1h/l`)** is kept in mode bit 3; the new `IBMCMD_GETMODES` ($7FE2) returns the mode word. DCTelnet will use it for `ESC O x` cursor keys in a following PR.
- **Fix:** `h`/`l` took the private prefix of parameter N from the Nth raw character, so `CSI ?1;7l` changed mode 7 without its `?`. The prefix now applies to the whole sequence.
- **Fix:** `CSI P` and `CSI @` in the last column did nothing.

## Testing

`make` in `ibmcon/` (68000 check). On an emulated A1200 with a local server sending each case: bright vs plain blue background (#0000FF / #0000DD), `AB|   GHIJ` after `CSI 4 X`, `ESC 8` back at row 6 column 20, column 80 blank after `CSI P` there, no wrap after `CSI ?1;7l`.

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01DmFJdpcHF7rhRU6XnPvz6P
