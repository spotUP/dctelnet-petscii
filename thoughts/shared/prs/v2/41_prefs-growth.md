---
date: 2026-09-28
topic: "PR #41: prefs: files of every version load quietly; 1.x files keep all their settings"
tags: [dctelnet, pr, v2.0-dev]
status: final
---

# #41 prefs: files of every version load quietly; 1.x files keep all their settings

Head `feature/pr-prefs-growth` on spotUP/dctelnet, base v2.0-dev.

**Based on v2.0-dev.** The first of a stack: the PRs after it build on it.

## Summary

LoadPrefs read sizeof(PrefsStruct) whatever the header said: any field
added to the struct made every existing file show "inconsistent data"
and "truncated data", and a DCTelnet 1.x file kept only its screen
mode, font and palette.

prefs_file.c (pure, host-tested) decodes the whole file:
- a v2 file of any data size: the fields it has, the newer ones 0 for
  ValidateAndInitPrefs() to default (fields are only appended);
- a 1.x file: screen, font, palette, paths, libraries, window geometry,
  terminal type, scrollback size and the option flags in their new
  meaning (hide -> enabled, Use Workbench -> not Full-screen, the console
  -> the ibmcon.device renderer, XEM -> the XEM renderer).
The debug requester on every legacy conversion is gone with the old
reader. tests/Makefile: the font check follows the fonts to
build/package/DCTelnet/Fonts.

## Testing on v2.0-dev

- Builds with vbcc for 68000 and 68020, warnings as errors; `cd tests && make` passes.
- Emulated A1200 (FS-UAE, driven through amiagent): A DCTelnet 1.x DCTelnet.Prefs (508 bytes) loads with its screen mode, font, colours and flags; the menus show its settings.

Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01DmFJdpcHF7rhRU6XnPvz6P
