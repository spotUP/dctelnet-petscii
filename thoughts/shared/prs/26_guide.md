---
date: 2026-09-28
topic: "PR #26 description -- AmigaGuide for the new features"
tags: [dctelnet, pr, docs]
status: final
---

**Stacked on #25.** Only the last commit belongs here.

## Summary

`docs/DCTelnet.guide` did not describe any of the features in #14-#25. This updates the AmigaGuide:

- **Features / Requirements**: PETSCII Mode, per-BBS settings, 256-colour AGA and RTG, the palette editor, Topaz Pro, 80 columns. OS 3.0 (V39) and Picasso96/CyberGraphX are listed as optional.
- **Installation**: DCTelnet's own Devs/Libs/Fonts drawers and the newest-copy rule; the new fonts; ibmcon.device 1.9 is needed for all features.
- **Address Book**: Comment, Login Macro (`\u \p \r \d \\`), Settings... with its four groups, Use Global Settings.
- **Options / Settings menus**: PETSCII Mode, Use Workbench (80x25, 80-column limit, global), Screen Mode/Font/Palette changes, Save Settings to Address Book Entry.
- **Known bugs**: #3 and #9 fixed in ibmcon 1.8 and 1.7; #11 improved by 1.9, not yet confirmed on every BBS.
- **History**: a section for these changes. I named it "1.9.1 PETSCII/RTG"; please rename it to the version you release them in.

The file stays Latin-1 (checked by a round trip); menu and gadget names match the program's own labels.

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01DmFJdpcHF7rhRU6XnPvz6P
