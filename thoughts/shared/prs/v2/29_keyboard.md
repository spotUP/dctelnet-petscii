---
date: 2026-09-28
topic: "PR #29: Keys: whole sequences, Home/End/Page keys, cursor key mode"
tags: [dctelnet, pr, v2.0-dev]
status: final
---

# #29 Keys: whole sequences, Home/End/Page keys, cursor key mode

Head `feature/pr-keyboard` on spotUP/dctelnet, base v2.0-dev.

**Based on v2.0-dev, stacked on #28.** Only the last commit belongs here.

This PR was first opened against `main`; it now carries the same feature ported onto v2.0-dev (its Prefs format, State bits and menus).

## Summary

- The key parser read only the first byte after CSI: Shift-F1 fired the
  F2 macro and then sent "~", Shift-cursor keys leaked "A" or "@".
  src/keys.c (host-tested) reads the whole MapRawKey sequence and names
  the key: F1-F10, Shift-F1-F10, cursor keys, Help, and the new-keyboard
  Insert, Page Up/Down, Home and End.
- Home, End, Page Up, Page Down and Insert send ANSI-BBS codes (ESC[H,
  ESC[K, ESC[V, ESC[U, ESC[@), or VT codes (ESC[1~ ...) with Terminal >
  VT Keys (APP_VT_KEYS; Keyboard group of an Address Book entry). On an
  A500/A1200 keyboard Shift+Up/Down is Page Up/Down and Shift+Left/Right
  Home/End.
- With the ibmcon.device renderer the cursor keys follow the host's
  cursor key mode (CSI ?1h, asked with IBMCMD_GETMODES, ibmcon 1.10):
  ESC O A-D. The other renderers keep ESC [ A-D.
- Old settings files keep VT Keys.

## Testing on v2.0-dev

- Builds with vbcc for 68000 and 68020, warnings as errors; `cd tests && make` passes.

<details><summary>Testing of the version for main (the same feature)</summary>

- Host tests: `cd test && make` (`test_keys`).
- Emulated A1200, keys injected through input.device, a local server recording the bytes: Shift-F1 nothing, Up `ESC[A`, Shift-Up `ESC[V`, Shift-Left and Home `ESC[H`, Insert `ESC[@`, Up after `CSI ?1h` `ESC O A`.

</details>

Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01DmFJdpcHF7rhRU6XnPvz6P
