---
date: 2026-09-28
topic: "PR #23: ibmcon 1.8: CloseDevice no longer frees DCTelnet's signal bit 31 (#3)"
tags: [dctelnet, pr, v2.0-dev]
status: final
---

# #23 ibmcon 1.8: CloseDevice no longer frees DCTelnet's signal bit 31 (#3)

Head `feature/pr-ibmcon-close-signal` on spotUP/dctelnet, base v2.0-dev.

**Based on v2.0-dev, stacked on #22.** Only the last commit belongs here.

This PR was first opened against `main`; it now carries the same feature ported onto v2.0-dev (its Prefs format, State bits and menus).

## Summary

The root of issue #3: ibmcon's UnitClose runs in the opener's task and
deleted the handler's command port there. The port's signal was allocated
in the handler's task (bit 31, a new process's first free bit), so
FreeSignal freed DCTelnet's bit 31 -- the one bsdsocket.library usually
holds -- and WaitSelect() stopped reporting data after a display change.
The handler now deletes its own port before replying to CMD_DIE.

DCTelnet's AllocSignal(31) reservation stays, relabelled: it only matters
for an ibmcon older than 1.8 still in memory and in use elsewhere.

Measured on an emulated A1200 with a test program that holds bit 31 and
opens and closes the device: 1.7 frees the bit, 1.8 leaves it allocated.

## Testing on v2.0-dev

- Builds with vbcc for 68000 and 68020, warnings as errors; `cd tests && make` passes.

<details><summary>Testing of the version for main (the same feature)</summary>

On an emulated A1200, a test program holds bit 31, opens ibmcon on a window and closes it:

```
ibmcon 1.7: bit 31 before close: allocated, after close: FREED
ibmcon 1.8: bit 31 before close: allocated, after close: allocated
```

</details>

Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01DmFJdpcHF7rhRU6XnPvz6P
