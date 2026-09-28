---
date: 2026-09-28
topic: "DCTelnet: the gaps against modern clients closed -- PRs #27-#39"
tags: [dctelnet, handoff, pr]
status: final
---

# Handoff 2026-09-28 (2) -- modern-client gaps, PRs #27-#39

## Task(s)

Owner: "what is missing from dc telnet that is expected from clients like
hyper term etc today?" -> plan `~/.claude/plans/soft-tumbling-raccoon.md`
(49 items, P0-P13), owner chose all of Tier 1 + 2 plus the tool bar icons.
All 13 phases are implemented and in PRs to bruno-frederic/dctelnet,
stacked on #26 (each branch contains the previous one):

| PR | Branch | Content | Verified |
|----|--------|---------|----------|
| #27 | feature/pr-wb-toolbar-icons | tool bar icons carry their palette (iconpens.c), Square/Wide sets, antialiased Lucide | rig both modes |
| #28 | feature/pr-ibmcon-1.10 | ibmcon 1.10: iCE, CSI X, ESC 7/8, DECCKM + GETMODES | rig |
| #29 | feature/pr-keyboard | keys.c: whole sequences, Home/End/Page/Insert, VT Keys, DECCKM | rig bytes |
| #30 | feature/pr-ibmcon-1.11 | ibmcon 1.11: screen buffer, READTEXT, blink | probe 17/17, screenshots |
| #31 | feature/pr-clipboard | mouse selection -> clipboard, Edit > Paste / Copy Screen | rig drag+paste |
| #32 | feature/pr-capture | Capture to File, Save Screen (.ans), F2 find; requester Return-twice fix | rig |
| #33 | feature/pr-timers | prefs ext block (DCS4) + redial, keep alive, connect timeout, non-blocking macro | rig |
| #34 | feature/pr-arexx | ARexx port DCTELNET.n, WAITFOR, macro \w"text" | rig with rx |
| #35 | feature/pr-rlogin | Rlogin (Synchronet auto-login) | rig handshake |
| #36 | feature/pr-charset | Character Set CP437/Amiga/UTF-8 | rig round trip |
| #37 | feature/pr-sound | Bell Flash/Sound/Off, ANSI music | text filtering only |
| #38 | feature/pr-wide | 132 Columns | rig NAWS |
| #39 | feature/pr-import | syncterm.lst import | rig |

Owner drive `~/Downloads/DCTelnet-PETSCII/DCTelnet`: build 85f9cfb, ibmcon
1.11, ToolBar/, Rexx/Login.rexx.

## Open (owner)

- SSH: research `thoughts/shared/research/2026-09-28_ssh-feasibility.md`,
  recommendation no-go for AmiSSL+libssh2; go/no-go + license question.
- Reset Screen owns Amiga-C; Copy has no shortcut (release copies).
- Owner tests: tool bar (both modes, pressed), iCE/blink on a real BBS,
  keys on a real BBS, selection+paste, capture/save screen, redial/keep
  alive on a real BBS, login macro \d and \w with an entry, ARexx
  Login.rexx, Rlogin to a Synchronet BBS, UTF-8 host, Bell Sound + ANSI
  music (ears), 132 columns on a wide screen, import of a real
  syncterm.lst.

## Learnings

- Rig: drive FS-UAE live with amiagent (Aminet comm/net/amiagent; client
  scratchpad `ami.py`); `uae_sound_output = interrupts`, never `none` (two
  queued audio.device writes froze the emulated machine with `none`).
- vbcc: `offsetof` takes no member path; struct padding is an error.
- ASL: typing a path with ':' into the File gadget leaves the gadget;
  UIACT settext on Drawer and File works.
- GW-BASIC PLAY: octave 3 starts at middle C (O3 A = 440 Hz).
