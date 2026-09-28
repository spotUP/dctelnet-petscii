---
date: 2026-09-28
topic: "PR #37: Bell as a sound or off; ANSI music"
tags: [dctelnet, pr, v2.0-dev]
status: final
---

# #37 Bell as a sound or off; ANSI music

Head `feature/pr-sound` on spotUP/dctelnet, base v2.0-dev.

**Based on v2.0-dev, stacked on #36.** Only the last commit belongs here.

This PR was first opened against `main`; it now carries the same feature ported onto v2.0-dev (its Prefs format, State bits and menus).

## Summary

- Terminal > Bell: Flash (the screen flashes, as before), Sound (a short
  tone through audio.device, src/sound.c) or Off. A BEL the BBS sends is
  handled before the console, also in PETSCII sessions.
- Terminal > ANSI Music (off by default) plays the tunes some BBSes send
  (ESC[M / ESC[N / ESC[| music strings, as SyncTERM plays them) instead
  of showing them as garbage; CSI M (Delete Lines) is not taken for
  music: ESC[M is a tune only when a music command follows it. src/ansimusic.c (pure, host-tested) parses the music strings,
  also split between network reads.
- The settings are the Bell and AnsiMusic fields of the PrefsStruct (from
  the reserved bytes); old files of the builds before v2.0 keep them.

## Testing on v2.0-dev

- Builds with vbcc for 68000 and 68020, warnings as errors; `cd tests && make` passes.

<details><summary>Testing of the version for main (the same feature)</summary>

- Host tests: `cd test && make` (`test_ansimusic`, `test_ansiscan`).
- Emulated A1200 with Paula emulated but silent: a tune between two texts and a BEL left `before the tune  after the tune bell` on screen, and `CSI 2M` after it still deleted two lines.
- The sound itself has not been listened to yet.

</details>

Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01DmFJdpcHF7rhRU6XnPvz6P
