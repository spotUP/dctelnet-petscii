---
date: 2026-09-28
topic: "PR #35 description -- rlogin"
tags: [dctelnet, pr]
status: final
---

**Stacked on #34.** Only the last commit belongs here.

## Summary

- **Options > Rlogin** (also in the Terminal group of an Address Book entry's Settings): after connecting, DCTelnet sends the rlogin login message (RFC 1282), then carries plain bytes, as a Raw Connection does.
  - The client user name is the entry's password and the server user name its user name. That is Synchronet's automatic login.
  - Without a password it sends the user name twice; without a user name, "guest".
  - The terminal type is the Telnet Display ID ("PETSCII" in PETSCII Mode).
- The server's NUL answer to the login message is not drawn.
- `TELNET_DATA()`: every data-path check of Raw Connection now also covers Rlogin: receive, IAC doubling of typed keys, paste, keep alive, and the transfer code in `Xfer.c`.
- `src/rlogin.c` is pure and host-tested.

Not included: the window-size exchange, which the server asks for with out-of-band data. Rlogin BBSes work without it.

## Testing

- Host tests: `cd test && make` (`test_rlogin`).
- Emulated A1200 against a local server: it received `\0guest\0guest\0VT102/38400\0`; its reply showed without the NUL, with a 255 byte kept as data.

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01DmFJdpcHF7rhRU6XnPvz6P
