---
date: 2026-09-28
topic: "PR #25: ANSI compatibility: answer Device Status Reports; Cursor Left/Right stop at the margins (#11)"
tags: [dctelnet, pr, v2.0-dev]
status: final
---

# #25 ANSI compatibility: answer Device Status Reports; Cursor Left/Right stop at the margins (#11)

Head `feature/pr-ansi-dsr-cursor` on spotUP/dctelnet, base v2.0-dev.

**Based on v2.0-dev, stacked on #24.** Only the last commit belongs here.

This PR was first opened against `main`; it now carries the same feature ported onto v2.0-dev (its Prefs format, State bits and menus).

## Summary

- DSR. A BBS asking where the cursor is (CSI 6 n) or whether the terminal
  is there (CSI 5 n) stalled (Absinthe animation, 20 For Beers
  countdown). A scanner (src/dsr.c, host-tested, including a request
  split between two network reads) finds the requests in the BBS's text;
  BbsWrite() draws the text up to the request, asks ibmcon where the
  cursor is (IBMCMD_GETCURSOR, ibmcon 1.9) and sends ESC[row;colR, or
  ESC[0n for CSI 5 n. The built-in renderer and XEM answer both
  themselves, so DCTelnet stays quiet there; console.device cannot tell
  the position. A request cut off by a disconnect is dropped.
- XEM's answers never reached the BBS: an XEM library sends them with
  xem_swrite(answer, -1), a NUL-terminated string, and xpr_swrite took
  the -1 as a length (AllocMem failed, nothing was sent). It now takes
  the string's length.
- CUB/CUF (ibmcon 1.9). Cursor Backward at column 1 went up to the end of
  the previous line, and Cursor Forward at the right edge wrapped to the
  next one; both now stop at the margin, as in ANSI.SYS and VT100.

## Testing on v2.0-dev

- Builds with vbcc for 68000 and 68020, warnings as errors; `cd tests && make` passes.

<details><summary>Testing of the version for main (the same feature)</summary>

- Host unit tests: `cd test && make` (`test_dsr`).
- On an emulated A1200 with a local server: `CSI 6 n` after `here` at row 5, column 10 is answered `ESC[5;14R`; `CSI 5 n` is answered `ESC[0n`; `CSI 79 D` and `CSI 20 C` stop at columns 1 and 80 of their row.

</details>

Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01DmFJdpcHF7rhRU6XnPvz6P
