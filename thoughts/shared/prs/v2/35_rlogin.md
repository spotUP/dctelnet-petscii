---
date: 2026-09-28
topic: "PR #35: Rlogin connections"
tags: [dctelnet, pr, v2.0-dev]
status: final
---

# #35 Rlogin connections

Head `feature/pr-rlogin` on spotUP/dctelnet, base v2.0-dev.

**Based on v2.0-dev, stacked on #34.** Only the last commit belongs here.

This PR was first opened against `main`; it now carries the same feature ported onto v2.0-dev (its Prefs format, State bits and menus).

## Summary

Terminal > Rlogin (and the Rlogin box of an Address Book entry's Terminal
settings, APP_RLOGIN) connects with the rlogin protocol (RFC 1282,
usually port 513) instead of telnet. BBSes such as Synchronet log the
user in directly: DCTelnet sends the entry's Username as the user name
and its Password as the password (no password: the user name twice; no
user name: "guest"), and the terminal type. The server's NUL answer is
not shown. src/rlogin.c (pure, host-tested) builds the login message.

An rlogin connection is a raw byte stream after the login message:
TELNET_DATA() (not Raw Connection, not rlogin) decides where telnet
codes (IAC) are read, doubled and sent, in the terminal, the keep alive,
pasting and the XPR transfers. Old settings files keep Rlogin.

## Testing on v2.0-dev

- Builds with vbcc for 68000 and 68020, warnings as errors; `cd tests && make` passes.
- Emulated A1200 (FS-UAE, driven through amiagent): An Address Book entry with Rlogin in its settings sends the Rlogin handshake to the server.

<details><summary>Testing of the version for main (the same feature)</summary>

- Host tests: `cd test && make` (`test_rlogin`).
- Emulated A1200 against a local server: it received `\0guest\0guest\0VT102/38400\0`; its reply showed without the NUL, with a 255 byte kept as data.

</details>

Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01DmFJdpcHF7rhRU6XnPvz6P
