---
date: 2026-09-28
topic: "PR #14 closed: in v2.0-dev at 702ab0b"
tags: [dctelnet, pr]
status: final
---

Closing: PETSCII Mode is in v2.0-dev (merged at 702ab0b, from f448799).

One commit of this branch did not go in: 2a584f6, "fonts: ship the PETSCII fonts as Amiga fonts, without their generator". In v2.0-dev the glyph files it read (`tools/glyphs/*.bin`) are gone, but `tools/gen_petscii_font.py` is still there and cannot run without them. If you want it removed, I can send that as a small PR of its own.

The PRs that followed (#15-#40, and #41-#44) are now ported onto v2.0-dev and target it.

Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01DmFJdpcHF7rhRU6XnPvz6P
