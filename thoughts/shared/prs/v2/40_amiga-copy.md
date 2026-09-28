---
date: 2026-09-28
topic: "PR #40: Copy on Right-Amiga+C, as on the Amiga"
tags: [dctelnet, pr, v2.0-dev]
status: final
---

# #40 Copy on Right-Amiga+C, as on the Amiga

Head `feature/pr-amiga-copy` on spotUP/dctelnet, base v2.0-dev.

**Based on v2.0-dev, stacked on #39.** Only the last commit belongs here.

This PR was first opened against `main`; it now carries the same feature ported onto v2.0-dev (its Prefs format, State bits and menus).

## Summary

A mouse selection no longer goes to the clipboard when the button is
released: Edit > Copy (Right-Amiga+C) copies it, as in a Shell window,
and the selection stays shown until the next click or the next text.
Reset Screen moves from Right-Amiga+C to Right-Amiga+Z. Copy reads the
text back from ibmcon.device, so it is disabled under the other
renderers, like Copy Screen and Save Screen.

## Testing on v2.0-dev

- Builds with vbcc for 68000 and 68020, warnings as errors; `cd tests && make` passes.

Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01DmFJdpcHF7rhRU6XnPvz6P
