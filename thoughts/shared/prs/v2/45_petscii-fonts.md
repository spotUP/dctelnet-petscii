---
date: 2026-09-28
topic: "PR #45: PETSCII fonts: ship the rendered fonts, drop their generator"
tags: [dctelnet, pr, v2.0-dev, fonts]
status: final
---

# #45 PETSCII fonts: ship the rendered fonts, drop their generator

Head `feature/pr-petscii-fonts` (759e950) on spotUP/dctelnet, base v2.0-dev.

**Based on v2.0-dev; independent of the other PRs.** The last commit of #14, which the v2.0-dev merge (702ab0b) did not include.

## Summary

Petscii.font and PetsciiLower.font already ship in `build/package/DCTelnet/Fonts/`, identical to the ones #14 delivered. The script that generated them, `tools/gen_petscii_font.py`, reads raw 8x8 glyph dumps (`tools/glyphs/*.bin`) that are not in v2.0-dev, so it cannot run. This PR:

- removes the generator: the rendered fonts are the source;
- adds `Fonts/Petscii.readme`, which describes the two fonts. The installer's `(fonts)` copy takes font files only, so the readme stays in the drawer. Package files are binary under `.gitattributes`, so here is its text:

```
Petscii.font / PetsciiLower.font
================================

Amiga bitmap fonts with the Commodore 64 character set, for DCTelnet's
PETSCII Mode. They are a free redraw of the C64 font.

  Petscii.font/8       upper case and graphics (the C64's default set)
  PetsciiLower.font/8  lower and upper case (the C64's shifted set)

Each is 16x8: the C64's 8x8 cells drawn twice as wide, so 40 columns fill
an Amiga hires line the way they fill a C64 screen. The glyphs are indexed
by the raw PETSCII byte.

Install: copy both .font files and their drawers to FONTS:, or leave them
in DCTelnet's own Fonts drawer -- DCTelnet looks there too. Without them
PETSCII Mode still works, with CP437 lookalike glyphs.

Check (DCTelnet's source tree): python3 tools/check_amiga_font.py
build/package/DCTelnet/Fonts/Petscii.font parses a font the way
diskfont.library loads it.
```

## Testing

- `python3 tools/check_amiga_font.py` on both fonts: `[OK] Petscii/8: 16x8, chars 0..255`, `[OK] PetsciiLower/8: 16x8, chars 0..255`.
- `git grep gen_petscii_font` and `git grep tools/glyphs` find no references left.
- Note: `tests/Makefile`'s check-fonts target in v2.0-dev still points at `../Fonts/`. #41 fixes that path.

Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01DmFJdpcHF7rhRU6XnPvz6P
