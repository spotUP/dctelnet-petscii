---
date: 2026-09-28
topic: "PR stack ported onto v2.0-dev; built-in renderer: windows, RTG, PETSCII"
tags: [dctelnet, v2.0-dev, port, retro32-term, handoff]
status: final
---

## Task(s)

1. The PR stack #15-#40 plus SSH, ported onto Bruno's `upstream/v2.0-dev` (702ab0b). The
   owner decided on 2026-09-28: "Port onto v2.0-dev now". Plan:
   `thoughts/shared/plans/2026-09-28-v2-port.md`. There is one squashed commit per old PR,
   and each branch builds on the one before.
2. Built-in renderer (retro32-term) todos from the owner: windows left on screen (B3),
   RTG (B2), PETSCII (B1). The owner chose "build it, offer to Bruno" and "terminal starts
   below the title bar".

Both are done and verified. The owner decides next: the test of the installed build
(below), and how the upstream PRs are replaced.

## Critical references

- Worktree `~/Code/dctelnet-v2`, branches `port/*` (local only, not pushed):

| Branch | Commit | Subject |
|---|---|---|
| port/prefs-growth | acbda99 | prefs: files of every version load quietly; 1.x files keep all their settings |
| port/per-entry-settings | 49489c7 | Per-Address-Book entry settings in override groups (#10) |
| port/petscii-session-display | 2e010f7 | PETSCII: the C64 display only during a PETSCII session |
| port/rtg-colours | 79b9377 | 256-colour AGA and RTG screens, ANSI colours on their own pens |
| port/terminal-size | 60d343b | Terminal size: Topaz Pro, 80 columns, window size to the BBS |
| port/drawer-components | 564299b | Newest ibmcon and libraries from DCTelnet's drawer; start-up screen |
| port/per-bbs-followups | de642a3 | Per-BBS settings: follow-ups from testing |
| port/palette-editor | 556e5e9 | An editor for the 16 ANSI colours; menu items in the menu text pen |
| port/ibmcon-insert-lines | ccfdadd | ibmcon 1.7: CSI L (Insert Lines) scrolls the whole region |
| port/ibmcon-close-signal | 2e8d7c0 | ibmcon 1.8: CloseDevice no longer frees DCTelnet's signal bit 31 (#3) |
| port/own-screen-font | 7f17560 | A font or palette change on the own screen reopens only the terminal |
| port/ansi-dsr-cursor | c49075a | ANSI compatibility: answer Device Status Reports; Cursor Left/Right stop at the margins (#11) |
| port/guide | b8c95b5 | Document the new features in DCTelnet.guide |
| port/wb-toolbar-icons | 953118c | Tool bar icons that suit every screen |
| port/ibmcon-1.10 | baded10 | feat(ibmcon): 1.10 -- iCE colours, CSI X, ESC 7/8, DECCKM mode query |
| port/keyboard | 551c58d | Keys: whole sequences, Home/End/Page keys, cursor key mode |
| port/ibmcon-1.11 | 5680bec | feat(ibmcon): 1.11 -- screen buffer, IBMCMD_READTEXT, blink |
| port/clipboard | 09841e9 | Select text with the mouse, Edit > Paste, Copy Screen |
| port/capture | 4f7efeb | Capture to file, save screen as ANSI, find in the scroll back |
| port/timers | ca71ee5 | Redial, keep alive, connect timeout; the login macro no longer blocks |
| port/arexx | db4eec7 | An ARexx port, and the login macro waits for a text |
| port/rlogin | 14e2fdc | Rlogin connections |
| port/charset | 76c1870 | Character Set: UTF-8 hosts, Amiga BBS text |
| port/sound | e712c09 | Bell as a sound or off; ANSI music |
| port/wide | 30c5531 | 132 Columns |
| port/import | e80d5b0 | Import a SyncTERM phone book |
| port/amiga-copy | e25031a | Copy on Right-Amiga+C, as on the Amiga |
| port/ssh | 2f471ad | SSH connections |
| port/builtin-layers | f75af2b | Built-in renderer: PETSCII sessions in the C64 font |

- retro32-term submodule branch `feat/layer-aware`: 2eee994, c35b289, 37282b8 (local only).
  Description for Bruno: `thoughts/shared/prs/retro32-term_layer-aware.md`.
- Test build (f75af2b, hash-checked): `~/Downloads/DCTelnet-PETSCII/DCTelnet-v2/`. It is a
  copy of the owner's drawer; their `DCTelnet/` is untouched.
- Ledger: `thoughts/spot/todos.md` (V0-V34, B1-B4, U1-U4).

## Recent changes

Three faults were found by the emulator runs and fixed in the branch where each one
came in; the chain was then re-picked:

- **A (V2, V3):** old DCS3/DCS4 site files never loaded on the Amiga. The read buffer
  was sized to the new 436-byte struct. Settings files are now read whole
  (`utils.c ReadWholeFile`).
- **B (V8, also in the OLD upstream PR #20):** an entry's settings leaked into the
  global ones. Connecting from the Address Book menu ran inside the menu handler, whose
  hand-change diff took the entry's settings for changes made by hand. The baseline is
  now reset in Begin/EndEntrySession.
- **C (V5):** renderer handlers decided the screen reopen from the new renderer bit
  (Bruno's code). Now they use the old one.

Other changes:

- **Built-in (port/builtin-layers):**
  - the engine draws directly when nothing covers the terminal, and through the
    window RastPort otherwise (and on RTG, 8 planes, or shared pens);
  - the terminal area is the window's text area;
  - PETSCII sessions use the C64 font at 40 columns.
- **Two older bugs fixed with it:**
  - a full reopen for the C64 display ran after the pending connect (blank screen);
  - a packet with a charset switch was drawn in the last charset (ibmcon too).

## Learnings

- A host test that decodes from a full buffer cannot catch a read-buffer bound. The
  rig can; so can a check of the Amiga-side reader.
- On a stock A500 every extra memory access competes with hires 4-plane DMA. A
  "two stores per glyph" cell buffer cost 3-8 %.
- Rig and emulator details are in memory `amiagent_rig_quirks_pointer_units_exec_serialises.md`:
  - amiagent clicks are Intuition pointer units (y doubled on a non-laced screen);
  - `exec` and a stuck ARexx call block the agent;
  - KS 1.3 needs `vc +kick13`.
- Bench harness: scratchpad `r32/` (session scratch). The numbers are recorded in the
  retro32-term PR description.

## Artifacts

- `thoughts/shared/plans/2026-09-28-v2-port.md`, `thoughts/shared/research/2026-09-28_v2-port.md`
- `thoughts/shared/prs/retro32-term_layer-aware.md`
- The old PR descriptions `thoughts/shared/prs/15_*.md` .. `39_*.md` still describe each
  port branch's feature.

## Submitted to upstream (2026-09-28, evening)

- Review pass (S1): findings and triage in the session scratchpad
  (v2prs/review_findings.md); every item marked FIX is fixed in its branch. Items left as
  they are: R1.3, R1.4, R1.9, R3.8, R4.8, and R1.5/R1.6 ("Known limits" in #16's body).
- All 30 port branches build (vbcc 68000 + 68020, warnings as errors) and pass `tests/`.
  The drop audit is unchanged.
- The tip passed the rig checks RV.T1-T6 (scratchpad v2rig/results.json):
  - E-clock WAITFOR;
  - SSH with a server rekey;
  - the built-in renderer's NAWS on a 1280-wide screen;
  - DECCKM kept over ESC 7/8, which failed on the pre-fix build;
  - blink;
  - the own-screen backfill.
- PRs #15-#40 were force-pushed with leases (backup branches `backup-2026-09-28-*` in
  dctelnet-v2) and retargeted to v2.0-dev. New PRs:
  - #41 prefs-growth;
  - #42 ssh;
  - #43 builtin-layers;
  - #44 console-fixes.
  All 30 are MERGEABLE. Bodies are in `thoughts/shared/prs/v2/`.
- #14 is closed with a note. Its last commit (2a584f6, which drops the font generator)
  never reached v2.0-dev; we offered to send it on its own.
- retro32-term: bruno-frederic/retro32-term#1 (spotUP:feat/layer-aware, 2aec2db), linked
  from #43.

## Next steps (in order)

1. Owner: test `DCTelnet-v2` (the tip build, 79ce5d7).
2. Follow review comments on #15-#44 and retro32-term#1. After each merge, bring
   v2.0-dev into the next branch.
3. The shipped `build/package/DCTelnet/Devs/ibmcon.device` is still the 1.6 binary on
   every branch; the fixed 1.11 builds only from `ibmcon/`. This needs the owner's
   decision (see Found).
4. Cleanup (V34): once the PRs are merged, remove the dctelnet-v2 worktree, the port/*
   branches and the backup-2026-09-28-* branches.

## Other notes

- `src/smakefile` got the SSH crypto objects, but is UNTESTED (no SAS/C here).
- Open, minor: a menu opened by another task between BuiltinCovered() and a write is a
  race; a window dragged by the user gets redrawn on the next event (smart refresh
  keeps it right in the meantime).
