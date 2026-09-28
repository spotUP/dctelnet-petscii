---
date: 2026-09-28
topic: "PR #28: feat(ibmcon): 1.10 -- iCE colours, CSI X, ESC 7/8, DECCKM mode query"
tags: [dctelnet, pr, v2.0-dev]
status: final
---

# #28 feat(ibmcon): 1.10 -- iCE colours, CSI X, ESC 7/8, DECCKM mode query

Head `feature/pr-ibmcon-1.10` on spotUP/dctelnet, base v2.0-dev.

**Based on v2.0-dev, stacked on #27.** Only the last commit belongs here.

This PR was first opened against `main`; it now carries the same feature ported onto v2.0-dev (its Prefs format, State bits and menus).

## Summary

renderer uses it).
- CSI ?33h/l switches iCE mode (SyncTERM): SGR 5 then gives a bright
  background, as ANSI art drawn for 16 background colours expects; SGR 25
  ends it.
- CSI X erases N characters (ECH). It takes the table slot of the CSI R
  stub: the dispatch table is full. The assembler now fails when the
  table or the $VER string before it moves.
- ESC 7 / ESC 8 save and restore the cursor like CSI s / CSI u; the
  restored position is clamped to a grid that shrank.
- CSI ?1h/l (DECCKM) is kept in mode bit 3; IBMCMD_GETMODES ($7FE2)
  returns the mode word so a client can send ESC O x cursor keys.
- h/l took the private prefix of parameter N from the Nth raw character,
  so CSI ?1;7l changed mode 7 without its '?'. The prefix now applies to
  the whole sequence.
- CSI P and CSI @ in the last column act on it (they did nothing).

Tested on an emulated A1200 with a local server: bright vs plain blue
background (#0000FF / #0000DD), "AB|   GHIJ" after CSI 4 X, ESC 8 back at
row 6 col 20, column 80 blank after CSI P there, no wrap after ?1;7l.

## Testing on v2.0-dev

- Builds with vbcc for 68000 and 68020, warnings as errors; `cd tests && make` passes.

<details><summary>Testing of the version for main (the same feature)</summary>

`make` in `ibmcon/` (68000 check). On an emulated A1200 with a local server sending each case: bright vs plain blue background (#0000FF / #0000DD), `AB|   GHIJ` after `CSI 4 X`, `ESC 8` back at row 6 column 20, column 80 blank after `CSI P` there, no wrap after `CSI ?1;7l`.

</details>

Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01DmFJdpcHF7rhRU6XnPvz6P
