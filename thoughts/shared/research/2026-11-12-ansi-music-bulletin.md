# ANSI music on bbs.uprough.net bulletins (2026-11-12) -- cause hunt, NOT root-caused

Status: client parser path and board emit path verified clean on the host. No dropping line found. Remaining causes need the live board / rig (not touched).

## Q1 Which bulletins
Only bulletin 56 (board repo /Users/spot/Code/AmiExpress/amiexpress-web/board/Bulletins/bull56.txt, Fur Elise, 785 bytes).
Sequence: `ESC [ M F T112 L16 MN O3 E D# ...` ended by 0x0E (ESC[M + MF, space-separated PLAY string, SO terminator, then CRLF ESC[0m). Grep for 1b5b4d / 1b5b4e / 1b5b7c over board/Bulletins/* finds only this file. The prose in the file ("ESC[M ... 0x0E") is plain text, not a sequence.
Handoff (amiexpress-web/handoff.md, 2026-11-04): bull56 was "copied to the live volume by hand (a new file never crosses)"; "Open sysop tests: B 56 in the web terminal and in DCTelnet". So whether it is on the live volume is UNVERIFIED.

## Q2 Replay through DCTelnet's parser
Built tests/ansimusic_dump.c + src/ansimusic.c (the code SoundFilter calls, src/DCTelnet.c:3479) and fed the whole bull56 bytes:
- one read: 204 note entries out, no ESC[M left in text (no stray Delete Line, no letters)
- one byte per read: identical (204)
- bytes with a CRLF every 80 chars inside the string (what the board's wrapLine does, see below): 206 entries, still plays
So the parser plays it. Max recv is 4096 (DCTelnet.c:401), the SoundFilter `n <= 4096` guard (:3488) is always satisfied. Default AnsiMusic=1 on first run (prefs_file.c:185); legacy prefs keep byte 450 (prefs_file.c:130). PETSCII_SESSION() is the only other bypass (:3488); not applicable to an ANSI session. RIP Auto does not sit before SoundFilter: both recv paths (DCTelnet.c:2441, :2565) call ShowText -> SoundFilter (:3596) first, so RIP detection never sees the sequence.

## Q3 Board side
No stripping of ESC[M or 0x0E on the bulletin path: io-file-display.ts displayResolvedFile -> io.ts emitText -> io-ansi-util.ts processRawText (only CSI 0x9B, FF, ESC[N p cursor codes, bare-[ completion) -> io-emit-lines.ts. Not applicable: utils/amiga-text.util.ts:82 strips 0x0E but is used only by dir-file.util.ts (DIR files), not bulletins.
Side effect (not a drop): io-emit-lines.ts:~85 wrapLine (line-wrap.util.ts) counts ESC[M as zero width (it matches the CSI regex with final byte M) and then hard-wraps the ~640-char music string every lineWrap columns with CRLF. The filter skips CR/LF in a string, so it still plays, but the wrap can cut a token (206 vs 204 entries). Cosmetic risk only.

## Likely remaining causes (unverified, in order)
1. Rig has no audio / audio.device cannot be opened: SoundOpen (src/sound.c SoundOpen) fails, Sound_Room()==0 and not busy -> DCTelnet.c:3491 discards the notes silently. fs-uae with audio off or channels taken gives exactly "no sound, no garbage".
2. The rig's saved prefs have AnsiMusic=0 (Terminal > ANSI Music menu item, DCTelnet.c:4922). Default is on only at first run; an old saved prefs file is not rewritten.
3. Bulletin 56 not on the live volume, or owner read a different bulletin (no other bulletin has music).
Check order for owner: menu Terminal > ANSI Music ticked? `B 56`? Does Bell=Sound beep (same sound.c path)? If the beep is silent too, it is the rig audio, not parsing.

## Owner of the fix
None found yet. If cause 1 or 2: DCTelnet (visible feedback: tell the user when audio cannot open, or show the menu tick state). Board wrapLine ESC[M handling is a minor board-side cleanup.
