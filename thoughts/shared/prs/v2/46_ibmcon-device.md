---
date: 2026-09-28
topic: "PR #46: Ship ibmcon.device 1.11, built from ibmcon/"
tags: [dctelnet, pr, v2.0-dev, ibmcon]
status: final
---

# #46 Ship ibmcon.device 1.11, built from ibmcon/

Head `feature/pr-ibmcon-device` (5c33494) on spotUP/dctelnet, base v2.0-dev.

**Based on v2.0-dev, stacked on #30** (ibmcon 1.11, whose source this builds). Only the last commit belongs here. It does not touch the files of #31-#44, so it merges in any order after #30.

## Summary

`build/package/DCTelnet/Devs/ibmcon.device` was still the 1.4 binary of the 1998 archive (`$VER: ibmcon.device 1.4 (Mar  9 1998)`). The guide says DCTelnet needs 1.11 "(included)", and `ibmcon/` holds the 1.11 source. So a user installing from the package got none of the 1.5-1.11 work: pen table, cursor position query, iCE colours, DECCKM, screen buffer, selection, blink.

- `Devs/ibmcon.device` is now the 1.11 build of `ibmcon/`, built with vasm as its Makefile does (plain 68000: `make check` passes). It is byte-identical to the device the full stack builds, since no later PR changes `ibmcon/`.
- `tests/`: `check-ibmcon` builds the device from `ibmcon/` and fails when the shipped one differs, so the two cannot drift apart again. It is skipped when vasm is not installed.
- `ibmcon/README.md` says the package ships this build.

## Testing

- `cd tests && make` passes. With the 1.4 binary put back, `check-ibmcon` fails (`differ: char 23`).
- Emulated A1200 (FS-UAE, driven through amiagent), with this device in DCTelnet's `Devs` drawer: DCTelnet reports "ibmcon.device 1.11 from DCTelnet's drawer"; DECCKM survives ESC 7/8 (the Up key sends ESC O A); SGR 5 text keeps blinking; an SSH session runs on it.

Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01DmFJdpcHF7rhRU6XnPvz6P
