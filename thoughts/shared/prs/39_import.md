---
date: 2026-09-28
topic: "PR #39 description -- import"
tags: [dctelnet, pr]
status: final
---

**Stacked on #38.** Only the last commit belongs here.

## Summary

**Connection > Import Address Book...** adds the entries of a SyncTERM phone book (`syncterm.lst`, an INI file) to `DCTelnet.Book`: name, address, port (the type's usual one when missing), user name and password.

- Entries already in the book (the same host and port) are skipped.
- Entries DCTelnet cannot connect to (SSH, modem, serial and the like) are skipped.
- RLogin and Raw entries get their own settings file that overrides only the Terminal group (Rlogin or Raw Connection).
- A requester reports how many were added and skipped.

`src/booklist.c` is pure and host-tested: sections, keys in any case, comments, CR LF, entries without an address, long names.

## Testing

- Host tests: `cd test && make` (`test_booklist`).
- Emulated A1200, driven through amiagent, with a list holding a Telnet, an RLogin and a Modem entry:
  - "Level 29" was added (port 23);
  - "Vertrauen" was added (port 513, user guest, `Sites/3.prefs`: `DCS4`, Terminal group, Rlogin);
  - the modem board was skipped;
  - the three existing entries stayed as they were.

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01DmFJdpcHF7rhRU6XnPvz6P
