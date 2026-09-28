---
date: 2026-09-28
topic: "PR #20: Per-BBS settings: follow-ups from testing"
tags: [dctelnet, pr, v2.0-dev]
status: final
---

# #20 Per-BBS settings: follow-ups from testing

Head `feature/pr-per-bbs-followups` on spotUP/dctelnet, base v2.0-dev.

**Based on v2.0-dev, stacked on #19.** Only the last commit belongs here.

This PR was first opened against `main`; it now carries the same feature ported onto v2.0-dev (its Prefs format, State bits and menus).

## Summary

- Full-screen and the screen mode are app-wide, never an entry setting:
  an entry saved on the Workbench opened the Workbench window on
  connect, and an entry's screen mode switched the user's screen. The
  Screen group is the font and the palettes.
- A setting changed by hand while connected is kept after the
  disconnect (carried into the global settings field by field, State
  bit by bit) and lasts for the run, but is never written to
  DCTelnet.Prefs (SitePrefs_HandChange / HandForSave).
- A Finger during an entry session waits for the display reopen, and
  its reply's lines start at column 1 (newline mode for the reply).
- Address Book: the Edit Profile frame encloses every field; deleting
  the first entry leaves a selection; typing in a field entered with Tab
  replaces its value.

Tests: tests/test_site_prefs.c (hand changes, every byte of the struct
carried), tests/test_listsel.c, tests/test_textedit.c.

## Testing on v2.0-dev

- Builds with vbcc for 68000 and 68020, warnings as errors; `cd tests && make` passes.

<details><summary>Testing of the version for main (the same feature)</summary>

- Host unit tests: `cd test && make` (group members, hand changes and what is saved, list selection after delete, first-key replace).
- Tested on an emulated A1200 and by the reporter on FS-UAE.

</details>

Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01DmFJdpcHF7rhRU6XnPvz6P
