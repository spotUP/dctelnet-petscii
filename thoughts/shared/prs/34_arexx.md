---
date: 2026-09-28
topic: "PR #34 description -- arexx"
tags: [dctelnet, pr]
status: final
---

**Stacked on #33** (WAITFOR's timeout uses its timer). Only the last commit belongs here.

## Summary

- **ARexx port `DCTELNET.1`** (`.2` and up for more running DCTelnets), opened when ARexx is running:
  - `CONNECT host [port]`, `DISCONNECT`
  - `SEND text` (`\r` Return, `\e` ESC), `SENDLN text`
  - `WAITFOR text [secs]`: the reply waits until the BBS has sent the text; RC 5 when it did not come in time (30 s default)
  - `CAPTURE file|OFF`, `GETSTATUS` (RESULT `CONNECTED host port` / `DISCONNECTED`), `QUIT`

  RC 10 for an unknown command or a missing argument. The port's signal is in the main loop's waits; messages still pending are answered when DCTelnet quits.
- **Login macro `\w"text"`**: waits until the BBS has sent the text (at most 30 s; then the macro stops). A fixed `\d` wait was the only option, and it sent the password too early to a slow BBS.
- `src/waitfor.c` (pure, host-tested) matches across network reads and skips colour codes inside the text (`ESC[1mPass ESC[0mword:`). `src/rexxcmd.c` (pure, host-tested) parses the commands.
- `Rexx/Login.rexx` example, installed with DCTelnet; a guide node documents the commands.

## Testing

- Host tests: `cd test && make` (`test_rexx`, `test_site_prefs` with `\w`).
- Emulated A1200, `rx` running a script against a local server:
  - `WAITFOR "Name:"` matched a prompt split over two reads with colour codes inside (RC 0);
  - both `SENDLN`s reached the server;
  - `GETSTATUS` returned `CONNECTED 127.0.0.1 2326`;
  - `WAITFOR "never" 3` returned RC 5, and an unknown command RC 10.

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01DmFJdpcHF7rhRU6XnPvz6P
