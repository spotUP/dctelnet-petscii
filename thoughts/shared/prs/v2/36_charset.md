---
date: 2026-09-28
topic: "PR #36: Character Set: UTF-8 hosts, Amiga BBS text"
tags: [dctelnet, pr, v2.0-dev]
status: final
---

# #36 Character Set: UTF-8 hosts, Amiga BBS text

Head `feature/pr-charset` on spotUP/dctelnet, base v2.0-dev.

**Based on v2.0-dev, stacked on #35.** Only the last commit belongs here.

This PR was first opened against `main`; it now carries the same feature ported onto v2.0-dev (its Prefs format, State bits and menus).

## Summary

Terminal > Character Set (and the Characters cycle of an Address Book
entry's Terminal settings) says what the BBS sends:
- IBM PC (CP437), the default: shown as it comes.
- Amiga (ISO-8859-1): Amiga BBS text; copied to the clipboard as it is.
- UTF-8: modern BBSes and Unix hosts. Decoded to the IBM set on the way
  to the console (the nearest glyph, box drawing exact), also when a
  character is split between two network reads; typed accented letters
  and pasted text go out as UTF-8 (the local echo shows them decoded).
src/charset.c is pure and host-tested. The setting is the Charset field
of the PrefsStruct; the struct now ends in explicit reserved bytes, so a
compiler adds no padding (every byte is a field that settings carry).
Old files of the builds before v2.0 keep it. Copying text trims empty
lines at the end.

## Testing on v2.0-dev

- Builds with vbcc for 68000 and 68020, warnings as errors; `cd tests && make` passes.

<details><summary>Testing of the version for main (the same feature)</summary>

- Host tests: `cd test && make` (`test_clip`: UTF-8 boxes, a character split between reads, typed Latin-1 as UTF-8, Amiga text copied as it is, blank rows dropped).
- Emulated A1200 in UTF-8 mode, with a local server sending a box and "café" in two pieces split inside a character. After Copy Screen and Right-Amiga+V the server received exactly `+-+ caf\xc3\xa9`.

</details>

Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01DmFJdpcHF7rhRU6XnPvz6P
