---
date: 2026-09-28
topic: "PR #22: ibmcon 1.7: CSI L (Insert Lines) scrolls the whole region"
tags: [dctelnet, pr, v2.0-dev]
status: final
---

# #22 ibmcon 1.7: CSI L (Insert Lines) scrolls the whole region

Head `feature/pr-ibmcon-insert-lines` on spotUP/dctelnet, base v2.0-dev.

**Based on v2.0-dev, stacked on #21.** Only the last commit belongs here.

This PR was first opened against `main`; it now carries the same feature ported onto v2.0-dev (its Prefs format, State bits and menus).

## Summary

Fixes the ABBS/MBBS fullscreen-editor glitch (lines not removed when the
editor scrolls up): ibmcon's Insert Lines used rows-1 as the bottom pixel
row of its scroll, a row count where Delete Lines uses rows*YSize-1, so
only the top pixel lines moved. Now rows*YSize-1, and an insert on the
last row clears it (N counts the cursor's row, as in CSI M).

Verified on the emulated A1200: ESC[5;1H ESC[3L moves line 05 to row 8
with rows 5-7 blank.

## Testing on v2.0-dev

- Builds with vbcc for 68000 and 68020, warnings as errors; `cd tests && make` passes.

<details><summary>Testing of the version for main (the same feature)</summary>

On an emulated A1200 with a local server sending 25 numbered lines, then `ESC[5;1H ESC[3L`: rows 5-7 are blank and line 05 is on row 8 (before the fix nothing moved).

</details>

Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01DmFJdpcHF7rhRU6XnPvz6P
