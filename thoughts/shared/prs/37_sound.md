---
date: 2026-09-28
topic: "PR #37 description -- sound"
tags: [dctelnet, pr]
status: final
---

**Stacked on #36.** Only the last commit belongs here.

## Summary

- **Options > Bell**: Flash (the default, ibmcon's `DisplayBeep`), Sound (a short tone through audio.device) or Off. BEL is taken out of the text unless the Bell is Flash, in PETSCII sessions too.
- **Options > ANSI Music** (off by default). Tunes a BBS sends as `ESC[M`, `ESC[N` or `ESC[|`, followed by a BASIC PLAY string and ended by `0x0E`, are played instead of shown.
  - Only `ESC[M` directly followed by music counts as music, so `ESC[2M` stays Delete Lines.
  - Without this, a tune's `0x0E` switched ibmcon to its upper character half, and the rest of the screen came out as graphics.
- `src/ansimusic.c` is pure and host-tested: pitch (octave 3 starts at middle C, as in GW-BASIC), tempo, lengths, dots, sharps and flats, octave changes, MN/ML/MS, a tune split between reads, and other sequences left untouched.
- `src/sound.c` queues one audio.device request per note on one channel. The allocation precedence is -40, so any other program wins the channel. A tune plays while DCTelnet goes on.

## Testing

- Host tests: `cd test && make` (`test_ansimusic`, `test_ansiscan`).
- Emulated A1200 with Paula emulated but silent: a tune between two texts and a BEL left `before the tune  after the tune bell` on screen, and `CSI 2M` after it still deleted two lines.
- The sound itself has not been listened to yet.

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01DmFJdpcHF7rhRU6XnPvz6P
