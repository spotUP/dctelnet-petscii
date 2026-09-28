---
date: 2026-09-28
topic: "PR #33: Redial, keep alive, connect timeout; the login macro no longer blocks"
tags: [dctelnet, pr, v2.0-dev]
status: final
---

# #33 Redial, keep alive, connect timeout; the login macro no longer blocks

Head `feature/pr-timers` on spotUP/dctelnet, base v2.0-dev.

**Based on v2.0-dev, stacked on #32.** Only the last commit belongs here.

This PR was first opened against `main`; it now carries the same feature ported onto v2.0-dev (its Prefs format, State bits and menus).

## Summary

- One timer.device request in the main loop, its signal in every wait
  (iconified too); a pure scheduler (src/ticks.c, host-tested) holds the
  timed jobs, on the E-clock: setting the clock does not move them.
- Connection > Connection Options...:
  - Redial: a connect refused or timed out is tried again N times, D
    seconds apart; Disconnect stops it.
  - Keep alive: after M minutes without a key sent, a telnet IAC NOP,
    which the BBS never shows. Off by default.
  - Connect timeout: a connect that takes longer than T seconds stops.
- The login macro's \d waits are timer events: the window stays live.
- Every user connect goes through DeferConnect (the display reopen first).
- The settings are four fields appended to the PrefsStruct
  (RedialTries, RedialDelay, AntiIdleMinutes, ConnectTimeout; 0 is each
  one's default); old files of the builds before v2.0 keep them.

## Testing on v2.0-dev

- Builds with vbcc for 68000 and 68020, warnings as errors; `cd tests && make` passes.

<details><summary>Testing of the version for main (the same feature)</summary>

- Host tests: `cd test && make` (`test_ticks`; `test_site_prefs` with a `DCS3` file loading after the struct grew).
- Emulated A1200, local server:
  - two refused connects each printed "Trying again in 4 seconds", and the third connected;
  - with 1 minute of Keep Alive the server received `FF F1` 60.1 s after the last byte;
  - a connect to 10.255.255.1 with a 5 s timeout printed "No answer in 5 seconds." and scheduled a redial.
- Not exercised end to end: the login macro's `\d` wait (the same timer path).

</details>

Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01DmFJdpcHF7rhRU6XnPvz6P
