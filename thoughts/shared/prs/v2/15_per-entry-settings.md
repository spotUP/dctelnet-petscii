---
date: 2026-09-28
topic: "PR #15: Per-Address-Book entry settings in override groups (#10)"
tags: [dctelnet, pr, v2.0-dev]
status: final
---

# #15 Per-Address-Book entry settings in override groups (#10)

Head `feature/per-entry-settings` on spotUP/dctelnet, base v2.0-dev.

**Based on v2.0-dev, stacked on #41.** Only the last commit belongs here.

This PR was first opened against `main`; it now carries the same feature ported onto v2.0-dev (its Prefs format, State bits and menus).

## Summary

Each Address Book entry can override groups of settings, Term 4.x
style; an overridden group comes from the entry while connected to it,
everything else stays global:
- Screen: screen mode, font, both palettes (AnsiColors and
  DeviceColors) and Full-screen;
- Terminal: PETSCII Mode, the renderer, XEM library, telnet terminal
  type, raw connection, local echo (+ XEM options);
- Keyboard: BackSpace/Del swap, Return sends CR+LF, the entry's own
  function keys;
- Transfer: XPR library and its options.
Also per entry: a comment and a login macro (\u username, \p password,
\r Return, \d one-second wait, \\ backslash), sent after connecting.

- The Address Book edit window gets Settings... (a Global/This Entry
  cycle per group, a summary, Use Current and the group's own gadgets;
  the Terminal group has a Renderer cycle) and Use Global Settings.
- Connection > Save Settings to Address Book Entry moves what was changed
  during a connection into that entry.
- Settings live in PROGDIR:Sites/<id>.prefs: a DCTFileHeader ('DCTS',
  version 1, the PrefsStruct size), groups, PrefsStruct, keys, macro. A
  PrefsStruct of another size loads as DCTelnet.Prefs does. Files of the
  builds before v2.0 (DCS1-DCS4, the 1.x struct) are converted.
- SavePrefs writes the global settings only: menu changes made while
  connected to such an entry last for that session.
- On disconnect the global settings come back; when the display
  differs it is reopened, and the connect waits for the reopen.

- In PETSCII Mode, text typed by a macro (function keys, the login
  macro) goes out case-swapped like typed text, and F1-F8 send the
  entry's macro when it has one (the C64 function key byte otherwise):
  an entry's own function keys work on a C64 board too.
- The palette editor works on the own screen only; on the Workbench
  the colours are the Workbench's.

Tests (tests/): group application, encode/decode of DCTS and of DCS1 and
DCS4 files, differing groups, summaries, login macro, key swapping.

## Testing on v2.0-dev

- Builds with vbcc for 68000 and 68020, warnings as errors; `cd tests && make` passes.
- Emulated A1200 (FS-UAE, driven through amiagent): Old site files (DCS2, DCS3, DCS4) load: a DCS4 entry with its own Terminal group connects with it (Rlogin handshake seen by a local server); after it, a plain entry connects with the global settings (nothing carried over).

<details><summary>Testing of the version for main (the same feature)</summary>

- Host unit tests: `cd test && make` (group application, file encode/decode incl. older formats, differing groups, summaries, login macro segments, key swapping).
- Tested on an emulated A1200 (FS-UAE) against several BBSes.

</details>

Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01DmFJdpcHF7rhRU6XnPvz6P
