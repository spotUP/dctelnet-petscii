---
date: 2026-09-28
topic: "PR #29 description -- keyboard"
tags: [dctelnet, pr]
status: final
---

**Stacked on #28** (it asks ibmcon 1.10 for the cursor key mode). Only the last commit belongs here.

## Summary

DCTelnet read one byte after the console's CSI and sent the rest of a key's sequence as text:

- Shift-F1 (`CSI 10~`) fired **F2's** macro and then sent `~`; Shift-F2..F10 likewise.
- Shift+Cursor keys (`CSI T`, `CSI S`, `CSI " A"`, `CSI " @"`) and Help (`CSI ?~`) sent stray characters.
- Home, End, Page Up/Down and Insert sent nothing, though BBS message editors and lightbar menus use them.

Changes:

- `src/keys.c` (pure, host-tested) reads each `MapRawKey` sequence whole and names the key. The extra keys of newer keyboards are also recognised by raw code (Insert `$47`, Page Up/Down `$48`/`$49`, Home `$70`, End `$71`), and `CSI 40~`-`45~` where a keymap maps them.
- Navigation keys send the codes BBSes expect (as SyncTERM: `ESC[H`, `ESC[K`, `ESC[V`, `ESC[U`, `ESC[@`). A new **VT Keys** option (Options menu, and the Keyboard group of an entry's Settings) sends VT codes instead (`ESC[1~`, `ESC[4~`, `ESC[5~`, `ESC[6~`, `ESC[2~`). On keyboards without these keys, Shift+Cursor Up/Down are Page Up/Down and Shift+Cursor Left/Right are Home/End.
- Cursor keys send `ESC O A`-`D` when the host set cursor key mode (`CSI ?1h`), asked from ibmcon 1.10 (`IBMCMD_GETMODES`).
- The key handler read `message->Code` after replying to the message; it now uses the copy.

## Testing

- Host tests: `cd test && make` (`test_keys`).
- Emulated A1200, keys injected through input.device, a local server recording the bytes: Shift-F1 nothing, Up `ESC[A`, Shift-Up `ESC[V`, Shift-Left and Home `ESC[H`, Insert `ESC[@`, Up after `CSI ?1h` `ESC O A`.

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01DmFJdpcHF7rhRU6XnPvz6P
