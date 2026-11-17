# RIP fixes: exact colours, one flush per write (2026-11-11)

Worktree /Users/spot/Code/DCTelnet/dctelnet-int87, branch local/int87-2026-11-10. Not pushed.

## Design
- Colour: VtImgColours (src/vtbuiltin.c) builds rgb[16] from the RIP palette and calls VtRipShowColours (src/DCTelnet.c), which loads the screen pens through SetPensRgb (extracted from ShowAnsiColours). Returns FALSE for non-fullscreen or true-colour screens; then the old nearest-pen path runs. Restore: VtRipPensRestore on RIP end, non-RIP picture, or LoadAnsiPalette (VtRipColoursLost).
- vtcon submodule: !|* now calls rip_pal_reset (engine/rip_vt.c). Submodule commit 77c3b2d on local branch local/rip-pal-reset.
- Slow: VtFeedRip flushed per command. Now one flush per write, plus mid-write only after VT_RIP_FLUSH_MS (100 ms, ReadEClock).

## Tests (tests/test_term_engine.c, host)
- Pass: rc=0.
- Fail-before: pens disabled fails the h2_a pen assertion; per-command flush restored fails the onFlushes == 1 assertion.
- The two new tests were not isolated in a fail-before run.
- Old per-command-flush assertions changed on purpose.

## Build
Amiga 68020 build rc=0 via make CPU=68020 build. Artifact is build/vbcc-68020-release/DCTelnet.68020 (no DCTelnet.020 name).

## Open
- RTG >8 bit and Workbench windows keep nearest-pen matching.
- ansiOwnPens real-hardware pen load untested (no rigs).
- RIP palette also recolours ANSI text while RIP active (as SyncTERM).
- No vtcon-side unit test for the palette reset.
