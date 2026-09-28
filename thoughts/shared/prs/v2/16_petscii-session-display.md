---
date: 2026-09-28
topic: "PR #16: PETSCII: the C64 display only during a PETSCII session"
tags: [dctelnet, pr, v2.0-dev]
status: final
---

# #16 PETSCII: the C64 display only during a PETSCII session

Head `feature/petscii-session-display` on spotUP/dctelnet, base v2.0-dev.

**Based on v2.0-dev, stacked on #15.** Only the last commit belongs here.

This PR was first opened against `main`; it now carries the same feature ported onto v2.0-dev (its Prefs format, State bits and menus).

## Summary

With PETSCII Mode on, DCTelnet used the C64 fonts all the time, so its
own texts (the start-up information, "Looking up ...", "Connection
closed") were unreadable C64 graphics. Now the C64 display is used only
during a PETSCII session: the normal font before connecting and after
disconnecting.
- On the ibmcon.device and console.device renderers the switch reopens
  only the console in the same window (OpenConsoleDevice and
  CloseConsoleDevice, split out of OpenDisplay/CloseDisplay); the screen
  and window stay. The built-in and XEM renderers take the full reopen.
- The built-in and XEM renderers draw with their own glyphs, so no C64
  font is opened for them: the PETSCII stream is translated to ANSI.
- DCTelnet's own messages printed while the C64 display is up are
  translated so they read correctly in the C64 upper/lower case sets.
- The "Connection closed" notice is printed again after the reopen that
  follows a disconnect.
- Finger is plain text: it ends a PETSCII session first.
- The console's I/O request is checked before use.

## Known limits

Known in this PR alone, removed later in the stack: a plain connect that fails (the Connect button, the command line) leaves the C64 display chosen until the next connect -- #33 sends every connect through one path that resets it; a Finger query on the C64 display is drawn in the C64 font and then cleared -- fixed in #20.

## Testing on v2.0-dev

- Builds with vbcc for 68000 and 68020, warnings as errors; `cd tests && make` passes.

<details><summary>Testing of the version for main (the same feature)</summary>

- Host unit tests: `cd test && make`.
- Tested on an emulated A1200 and on real hardware against a PETSCII board (amiexpress-web).

</details>

Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01DmFJdpcHF7rhRU6XnPvz6P
