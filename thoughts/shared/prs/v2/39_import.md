---
date: 2026-09-28
topic: "PR #39: Import a SyncTERM phone book"
tags: [dctelnet, pr, v2.0-dev]
status: final
---

# #39 Import a SyncTERM phone book

Head `feature/pr-import` on spotUP/dctelnet, base v2.0-dev.

**Based on v2.0-dev, stacked on #38.** Only the last commit belongs here.

This PR was first opened against `main`; it now carries the same feature ported onto v2.0-dev (its Prefs format, State bits and menus).

## Summary

DC Telnet > Import Address Book... reads a SyncTERM phone book
(syncterm.lst) and adds its telnet, Rlogin and raw entries to the
Address Book, with name, host, port, user name and password. Rlogin and
raw entries get settings of their own (the Terminal group, with the
APP_RLOGIN or APP_RAW_CONNECTION bit). Entries already in the book (same
host and port) are skipped; the requester at the end says how many were
added, skipped, and of a kind DCTelnet does not have (SSH, modem,
serial). src/booklist.c parses the list and is host-tested.

## Testing on v2.0-dev

- Builds with vbcc for 68000 and 68020, warnings as errors; `cd tests && make` passes.

<details><summary>Testing of the version for main (the same feature)</summary>

- Host tests: `cd test && make` (`test_booklist`).
- Emulated A1200, driven through amiagent, with a list holding a Telnet, an RLogin and a Modem entry:
  - "Level 29" was added (port 23);
  - "Vertrauen" was added (port 513, user guest, `Sites/3.prefs`: `DCS4`, Terminal group, Rlogin);
  - the modem board was skipped;
  - the three existing entries stayed as they were.

</details>

Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01DmFJdpcHF7rhRU6XnPvz6P
