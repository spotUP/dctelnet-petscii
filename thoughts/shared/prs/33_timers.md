---
date: 2026-09-28
topic: "PR #33 description -- timers"
tags: [dctelnet, pr]
status: final
---

**Stacked on #32.** The last two commits belong here.

## Summary

**Settings > Connection Options** (0 turns a setting off):

- **Redial Attempts / Seconds Between Attempts.** A connect that was refused, timed out or found no route is tried again. An Address Book entry's session (its own settings) is kept for the retry. Connection > Disconnect stops a waiting redial.
- **Keep Alive After Minutes.** After that long without a byte sent, a telnet NOP (`IAC NOP`, which the BBS does not show) keeps idle-timeout BBSes from hanging up.
- **Connect Timeout, Seconds.** The Connecting window's task has a `timer.device` request of its own and stops the connect as its Abort button does.

Underneath:

- **Settings extension block** (first commit). The settings struct gains 64 reserved bytes; a new multi-valued setting takes bytes from them, 0 meaning its default, so the struct and the entry files keep their size. The sizes are checked at compile time. Entry files are written as `DCS4`; `DCS3` files still load. `LoadPrefs()` zeroes the struct first, so a shorter (older) file always means defaults.
- `src/ticks.c` (pure, host-tested) keeps the deadlines. The main loop has one `timer.device` request for exactly the time to the earliest, with its signal in both waits; with nothing pending there is no request.
- **The login macro's `\d` waits were `Delay(50)` in the main loop:** the window froze meanwhile. They are timer events now.
- **Every connect runs through `DeferConnect()`/`RunPendingConnect()`.** The command line, Connection > Connect and the Address Book called `BeginServerConnection()` directly when no display reopen was needed, so a redial had nothing to repeat.
- The Connecting task signals the main task once, after its window and timer are gone.

## Testing

- Host tests: `cd test && make` (`test_ticks`; `test_site_prefs` with a `DCS3` file loading after the struct grew).
- Emulated A1200, local server:
  - two refused connects each printed "Trying again in 4 seconds", and the third connected;
  - with 1 minute of Keep Alive the server received `FF F1` 60.1 s after the last byte;
  - a connect to 10.255.255.1 with a 5 s timeout printed "No answer in 5 seconds." and scheduled a redial.
- Not exercised end to end: the login macro's `\d` wait (the same timer path).

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01DmFJdpcHF7rhRU6XnPvz6P
