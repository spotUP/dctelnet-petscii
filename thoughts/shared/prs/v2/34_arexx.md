---
date: 2026-09-28
topic: "PR #34: An ARexx port, and the login macro waits for a text"
tags: [dctelnet, pr, v2.0-dev]
status: final
---

# #34 An ARexx port, and the login macro waits for a text

Head `feature/pr-arexx` on spotUP/dctelnet, base v2.0-dev.

**Based on v2.0-dev, stacked on #33.** Only the last commit belongs here.

This PR was first opened against `main`; it now carries the same feature ported onto v2.0-dev (its Prefs format, State bits and menus).

## Summary

- ARexx port DCTELNET.1 (.2 and up for more running DCTelnets), opened
  when ARexx is running:
  - CONNECT host [port], DISCONNECT
  - SEND text (\r Return, \e ESC), SENDLN text
  - WAITFOR text [secs]: the reply waits until the BBS has sent the text;
    RC 5 when it did not come in time (30 s default)
  - CAPTURE file|OFF, GETSTATUS (RESULT CONNECTED host port /
    DISCONNECTED), QUIT
  RC 10 for an unknown command or a missing argument. The port's signal
  is in the main loop's waits; messages still pending are answered when
  DCTelnet quits.
- Login macro \w"text": waits until the BBS has sent the text (at most
  30 s; then the macro stops). A fixed \d wait was the only option, and
  it sent the password too early to a slow BBS.
- src/waitfor.c (pure, host-tested) matches across network reads and
  skips colour codes inside the text; src/rexxcmd.c (pure, host-tested)
  parses the commands.
- Rexx/Login.rexx example, installed with DCTelnet; a guide node
  documents the commands.

## Testing on v2.0-dev

- Builds with vbcc for 68000 and 68020, warnings as errors; `cd tests && make` passes.
- Emulated A1200 (FS-UAE, driven through amiagent): CONNECT, SENDLN, WAITFOR and CAPTURE from ARexx against a local server.

<details><summary>Testing of the version for main (the same feature)</summary>

- Host tests: `cd test && make` (`test_rexx`, `test_site_prefs` with `\w`).
- Emulated A1200, `rx` running a script against a local server:
  - `WAITFOR "Name:"` matched a prompt split over two reads with colour codes inside (RC 0);
  - both `SENDLN`s reached the server;
  - `GETSTATUS` returned `CONNECTED 127.0.0.1 2326`;
  - `WAITFOR "never" 3` returned RC 5, and an unknown command RC 10.

</details>

Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01DmFJdpcHF7rhRU6XnPvz6P
