---
date: 2026-09-24
topic: "PR #14 to bruno-frederic/dctelnet: PETSCII Mode"
tags: [dctelnet, petscii, pr]
status: final
---

https://github.com/bruno-frederic/dctelnet/pull/14 (head spotUP:feature/petscii-mode @ f448799)

## Summary

Adds **Options > PETSCII Mode**, so DCTelnet can connect to Commodore 64 (PETSCII) BBSes and show them with the real C64 character set, colours and reverse video.

With the mode on:

- DCTelnet answers the telnet terminal-type query with `PETSCII`, so a PETSCII-aware board switches to its C64 output.
- Incoming PETSCII is translated for `ibmcon.device`. Printable bytes go to the new `Petscii.font` / `PetsciiLower.font`, swapped on the C64 charset-switch codes. Control codes become ANSI sequences: cursor moves, HOME, CLR, DELETE, BELL, CR, the 16 colours and reverse video.
- Cursor keys, F1-F8, Backspace/Delete and typed characters are sent as PETSCII.
- If the fonts are not installed, PETSCII Mode still works with CP437 lookalike glyphs and prints a notice on connect.

## Commits

1. **C64 Petscii fonts and their generator.** Two 16x8 strike fonts, `tools/gen_petscii_font.py` to rebuild them, and `tools/check_amiga_font.py` to verify a font loads the way diskfont.library reads it. The checker also passes every font in the official archive.
2. **The PETSCII translator.** Platform-independent C modules with host unit tests (`cd test && make`).
3. **PETSCII Mode in DCTelnet.** The menu item, TTYPE, receive path, font loading and keyboard.

## Notes on ibmcon.device

Two behaviours shaped the translator:

- **No SGR 27.** ibmcon emulates ANSI.SYS, which has no SGR 27 (reverse off) or SGR 22 (bold off). Colour and reverse changes are therefore always rebuilt from SGR 0.
- **Immediate wrap.** ibmcon wraps right after the last column. With the 16-pixel font the window is exactly 40 columns, so DCTelnet turns the console's auto-wrap off (`CSI ?7l`) in PETSCII Mode and the translator does the C64 wrap itself.

Toggling the mode reopens the display, as Settings > Font does, because the console takes its cell size from the font when it is opened.

## Licensing -- please review

- **LGPL tables.** `src/petscii_fallback.*` and `src/petscii_keymap.*` port tables from SyncTerm's `cterm_petscii.h` (Rob Swindell, LGPLv2+) and keep that notice. DCTelnet itself has no license, so please decide whether statically linking LGPL code fits the repository.
- **Fonts: license unresolved.** The glyphs come from SyncTerm's `src/conio/allfonts.c`, which has no license header. The FreeBSD syscons provenance SyncTerm's docs claim could not be verified; details are in `Fonts/README.txt`. The fonts are optional: without them the mode uses CP437 lookalikes, so dropping the `Fonts/Petscii*` files from this PR does not break anything.

## Testing

- Host unit tests: `cd test && make`. They cover the translator, the tables, the font loader check, and a guard against U+FFFD in `src/` (DCTelnet.c's 8-bit strings do not survive a UTF-8 editor round trip).
- Built with vbcc against NDK 3.2R4 for 68000 and 68020.
- Tested on an emulated A1200 (FS-UAE) against an amiexpress-web board: the C64 title screen with the reversed logo, colours and 40-column layout matches SyncTerm.
- Not yet exercised on hardware: the upper/lower charset swap.

Not included: RUN/STOP key mapping, INSERT, cursor wrap at the screen edge, and the C64 blue background colour.

Separate from this PR: `src/aos68k` uses Windows-only `%%VBCC%%` placeholders and `del`, so building on macOS/Linux needs local edits. I can open a separate PR for that if you like.

🤖 Generated with [Claude Code](https://claude.com/claude-code)
