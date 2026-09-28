---
date: 2026-09-28
topic: "PR #44: console.device renderer: colours, background and the C64 charsets"
tags: [dctelnet, pr, v2.0-dev]
status: final
---

# #44 console.device renderer: colours, background and the C64 charsets

Head `feature/pr-console-fixes` on spotUP/dctelnet, base v2.0-dev.

**Based on v2.0-dev, stacked on #43.** Only the last commit belongs here.

## Summary

Three faults of the console.device renderer, found with PETSCII on it:

- Red and white were swapped. console.device draws plain text in pen 1
  and maps SGR 3n/4n to pen n; its palette (DeviceColors, ibmcon's
  order) has white on pen 1 so that plain text is white -- so SGR 31
  (red) drew white and SGR 37 red. No palette order suits both: the
  colours of SGR sequences are rewritten on the way to it instead, 1 and
  7 swapped as ibmcon does itself (Ansi_Swap17 in ansiscan.c: streaming,
  a sequence split between reads is held, other sequences such as CSI
  31;37H and 38;5;n are left alone; host-tested).
- A grey background after switching from ibmcon on a 256-colour screen:
  the screen was not reopened, and ibmcon lays out 32+ colour screens
  with Intuition's pens 0-7 (grey pen 0). A renderer switch now reopens
  the screen whenever the pen layout differs (ScreenPenLayout), not by
  a fixed list of renderers.
- The C64 lower-case set never showed: console.device draws with its own
  copy of the font (ConUnit cu_Font, public in devices/conunit.h), taken
  at OpenDevice. Both C64 fonts have the same cell, so the charset
  switch sets it there too.

Checked on the emulated A1200 (own 256-colour screen, ibmcon, then
console.device, a PETSCII board): black background, white and red where
they belong, both C64 charsets.

## Testing on v2.0-dev

- Builds with vbcc for 68000 and 68020, warnings as errors; `cd tests && make` passes.
- Emulated A1200 (FS-UAE, driven through amiagent): console.device on a 256-colour own screen after ibmcon, with a PETSCII board and an ANSI one: black background, red and white where they belong, both C64 charsets.

Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01DmFJdpcHF7rhRU6XnPvz6P
