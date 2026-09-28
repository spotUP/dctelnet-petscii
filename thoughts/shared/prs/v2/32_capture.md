---
date: 2026-09-28
topic: "PR #32: Capture to file, save screen as ANSI, find in the scroll back"
tags: [dctelnet, pr, v2.0-dev]
status: final
---

# #32 Capture to file, save screen as ANSI, find in the scroll back

Head `feature/pr-capture` on spotUP/dctelnet, base v2.0-dev.

**Based on v2.0-dev, stacked on #31.** Only the last commit belongs here.

This PR was first opened against `main`; it now carries the same feature ported onto v2.0-dev (its Prefs format, State bits and menus).

## Summary

- DC Telnet > Capture to File... writes what the BBS sends (after telnet
  decoding, before the console) to a file, asking before overwriting; a
  check mark in the menu while it runs (kept over a display reopen);
  again to stop.
- DC Telnet > Save Screen... saves the current screen as an ANSI file,
  rebuilt from ibmcon's screen buffer (src/ansiscan.c, host-tested): with
  the ibmcon.device renderer only, disabled with the others.
- The scroll back window finds text with F2, and F2 again finds the next
  match, searching forward and wrapping.
- Requesters: Return in a string field ended it only on the second press.

## Testing on v2.0-dev

- Builds with vbcc for 68000 and 68020, warnings as errors; `cd tests && make` passes.
- Emulated A1200 (FS-UAE, driven through amiagent): ARexx CAPTURE to a file across a page and later text: the file holds both.

<details><summary>Testing of the version for main (the same feature)</summary>

- Host tests: `cd test && make` (`test_ansiscan`: parameter bytes, case-blind find, screen row to ANSI with iCE backgrounds and ibmcon's unit pen order).
- Emulated A1200, menus and requesters driven through input.device:
  - the capture file holds exactly the bytes the local server sent after capture was switched on;
  - the saved screen starts `ESC[0;1;31;44mCAPTURED` and keeps the raw CP437 `C4 C4`;
  - F2, `needle`, one Return: "find the Needle here" is the top line of the Scroll Back.

</details>

Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01DmFJdpcHF7rhRU6XnPvz6P
