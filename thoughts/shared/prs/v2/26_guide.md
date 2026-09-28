---
date: 2026-09-28
topic: "PR #26: Document the new features in DCTelnet.guide"
tags: [dctelnet, pr, v2.0-dev]
status: final
---

# #26 Document the new features in DCTelnet.guide

Head `feature/pr-guide` on spotUP/dctelnet, base v2.0-dev.

**Based on v2.0-dev, stacked on #25.** Only the last commit belongs here.

This PR was first opened against `main`; it now carries the same feature ported onto v2.0-dev (its Prefs format, State bits and menus).

## Summary

The features of the ports so far, in the guide: per-BBS settings (the
Address Book's Settings... window and its four groups, the login macro,
Connection > Save Settings to Address Book Entry), PETSCII Mode, 256
colours and RTG, the ANSI colours editor, Topaz Pro, the 80-column
terminal, the ibmcon source and versions 1.5-1.9, and the bug fixes.
They are listed in the 2.0 history entry, below its TODO for the v2.0
work itself. The menu chapters still describe the 1.9 menus (Options,
Settings); only the text added here uses the 2.0 menu names.

## Testing on v2.0-dev

- Builds with vbcc for 68000 and 68020, warnings as errors; `cd tests && make` passes.

Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01DmFJdpcHF7rhRU6XnPvz6P
