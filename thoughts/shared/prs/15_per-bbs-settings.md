Closes #10.

**Stacked on #14** (PETSCII Mode): this branch contains #14's commits; please review #14 first. Only the commits after it belong to this PR.

## Summary

Each Address Book entry can override groups of settings, Term 4.x style. An overridden group comes from the entry while connected to it; everything else stays global:

- **Screen**: screen mode, font, palette and Use Workbench (narrowed to font and palette in #20, after testing showed a BBS should not move DCTelnet between screens).
- **Terminal**: PETSCII Mode, XEM library and display driver, telnet terminal type, raw connection, local echo (+ XEM options).
- **Keyboard**: BS/DEL swap, Return = CR+LF, and the entry's own function keys.
- **Transfer**: XPR library and its options.

Also per entry: a comment and a **login macro** (`\u` username, `\p` password, `\r` Return, `\d` one-second wait, `\\` backslash), sent after connecting.

## How it works

- The edit window gets **Settings...** (one Global/This Entry cycle per group, a summary, *Use Current* and the group's own gadgets) and **Use Global Settings**.
- **Settings > Save Settings to Address Book Entry** moves what was changed during a connection into that entry.
- Settings live in `PROGDIR:Sites/<id>.prefs` next to the Address Book; `BookStruct` gains a `settingsId` and a comment in its reserved space (still 256 bytes, checked at compile time).
- On disconnect the global settings come back. When the display differs, it is reopened; the connect is queued until the reopen is done.

## Testing

- Host unit tests: `cd test && make` (group application, file encode/decode incl. older formats, differing groups, summaries, login macro segments, key swapping).
- Tested on an emulated A1200 (FS-UAE) against several BBSes.

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01DmFJdpcHF7rhRU6XnPvz6P
