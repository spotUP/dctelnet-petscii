---
date: 2026-09-28
topic: "PR #36 description -- charset"
tags: [dctelnet, pr]
status: final
---

**Stacked on #35.** Only the last commit belongs here.

## Summary

The terminal shows CP437, and copying assumed it. So a UTF-8 BBS drew each box piece as three wrong characters, and text copied from an Amiga BBS (ISO-8859-1) had its accented letters turned into CP437 lookalikes.

**Options > Character Set** (also "Characters" in the Terminal group of an entry's Settings) says what the BBS sends:

- **IBM PC (CP437)**: as before, and the default, so nothing changes unless it is set.
- **Amiga (ISO-8859-1)**: copied text is kept as it is.
- **UTF-8**: shown in the IBM set, with box drawing exact and `?` for what CP437 lacks. A character split between network reads still decodes. Accented letters typed or pasted are sent as UTF-8.

Copy Screen also drops the blank rows below the text; it pasted a Return for each.

The conversions are pure and host-tested (`charset.c`, `clip.c`). The capture file keeps the bytes as received.

## Testing

- Host tests: `cd test && make` (`test_clip`: UTF-8 boxes, a character split between reads, typed Latin-1 as UTF-8, Amiga text copied as it is, blank rows dropped).
- Emulated A1200 in UTF-8 mode, with a local server sending a box and "café" in two pieces split inside a character. After Copy Screen and Right-Amiga+V the server received exactly `+-+ caf\xc3\xa9`.

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01DmFJdpcHF7rhRU6XnPvz6P
