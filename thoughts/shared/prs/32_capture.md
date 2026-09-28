---
date: 2026-09-28
topic: "PR #32 description -- capture"
tags: [dctelnet, pr]
status: final
---

**Stacked on #31.** Only the last commit belongs here.

## Summary

- **DCTelnet > Capture to File...** Everything the BBS sends -- the telnet codes taken out, before any translation -- is written to a file as it arrives, until the item is chosen again. The menu shows a check mark while capturing.
- **DCTelnet > Save Screen...** The terminal as it is, as an `.ans` file: the CP437 characters, an SGR sequence where the colours or attributes change (iCE backgrounds as `5`), trailing blanks dropped, CR LF line ends. It is read back from ibmcon 1.11's screen buffer (#30).
- **F2 in the Scroll Back** finds text, ignoring case: from the line after the top one, wrapping round; the line found is scrolled to the top. F2 again, then Return, finds the next one.
- The save requesters open on DCTelnet's drawer (`PROGDIR:`). `AskSavePath()` and `OpenNewFileAsking()` are shared with the scroll back's F5 save.
- **Fix:** Return in a text requester's field ends the field with a `GADGETUP`, which `GetStringRequester()` ignored, so Connect, Find and the other text requesters needed Return twice. Tab still only leaves the field.
- **Fix:** the scroll back's copy of the text read CSI parameters only up to `;`, so `CSI ?25h` left `25h` in it. `Ansi_ParamsEnd()` takes every ECMA-48 parameter byte (`0`-`?`).

## Testing

- Host tests: `cd test && make` (`test_ansiscan`: parameter bytes, case-blind find, screen row to ANSI with iCE backgrounds and ibmcon's unit pen order).
- Emulated A1200, menus and requesters driven through input.device:
  - the capture file holds exactly the bytes the local server sent after capture was switched on;
  - the saved screen starts `ESC[0;1;31;44mCAPTURED` and keeps the raw CP437 `C4 C4`;
  - F2, `needle`, one Return: "find the Needle here" is the top line of the Scroll Back.

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01DmFJdpcHF7rhRU6XnPvz6P
