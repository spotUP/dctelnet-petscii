---
date: 2026-09-24
topic: "DCTelnet PETSCII Mode: hardware rounds 4-7, repo loss and reconstruction, amiexpress-web TTYPE race, upstream PR #14"
tags: [amiga, dctelnet, petscii, telnet, ibmcon, amiexpress-web, syncterm, vbcc]
status: final
---

# Session handoff -- DCTelnet PETSCII Mode

Supersedes the earlier 2026-09-24 handoff, which was lost with the original
repository (see "Repository loss" below).

## Task(s)

1. Finish PETSCII emulation in DCTelnet (native Amiga telnet client, fork of
   bruno-frederic/dctelnet) so it renders amiexpress-web's C64 screens.
   **Done and owner-verified**: "it works! and it's beautiful!" -- FS-UAE,
   emulated A1200, local amiexpress-web board, build `167bbde`.
2. Fix amiexpress-web so a slow PETSCII client is detected as a C64.
   **Done, pushed, live** (`e25b8edd7`, contained in live revision `5688920`).
3. Recover the destroyed repository. **Done** (reconstruction, history lost).
4. Private backup repo. **Done**: https://github.com/spotUP/dctelnet-petscii
5. Upstream PR. **Open**: https://github.com/bruno-frederic/dctelnet/pull/14,
   waiting for Bruno's review.

## Critical References

- Working repo: `/Users/spot/Code/dctelnet-petscii-recovered`
  - `feature/petscii-emulation` -- private working branch, remote `origin`
    = `spotUP/dctelnet-petscii` (private).
  - `feature/petscii-mode` -- the clean 3-commit PR branch, remote `fork` =
    `spotUP/dctelnet` (public fork). Head `f448799`.
  - `main` -- upstream `c244bc6`; remote `upstream` = bruno-frederic/dctelnet.
- Build: `cd src && VBCC=/opt/homebrew/opt/vbcc make`. `src/aos68k` carries
  UNCOMMITTED Mac-local paths (vbcc prefix, `.ndk/Include_H`, `rm -f`); the
  NDK is extracted into `.ndk/` from `~/Downloads/NDK3.2R4.lha` (git-excluded).
- Tests: `cd test && make` (dispatch + tables, U+FFFD guard, font loader check).
- Fonts: `tools/gen_petscii_font.py` regenerates `Fonts/Petscii*/8`
  byte-identically from `tools/glyphs/*.bin`; `tools/check_amiga_font.py`.
- ibmcon.device annotated source: `/Users/spot/Code/ibmcon`
  (`ibmcon.device.bugfixed.asm`). DCTelnet ships the ORIGINAL 1.4 binary
  (md5 differs from the bug-fixed build).
- Owner's test rig: FS-UAE mounts `~/Downloads/DCTelnet-PETSCII` as
  `hard_drive_1`. New builds must be copied THERE (`DCTelnet/DCTelnet`,
  `DCTelnet/DCTelnet.020`, `DCTelnet/Fonts/Petscii*`); check the
  `build <hash>` string in the binary.
- Deliverable folder: `~/Desktop/DCTelnet-PETSCII/` (build, src copies,
  fonts, `DCTelnet-PETSCII.lha`, README.txt with the test steps).
- SyncTerm reference clone (session scratchpad, will not survive):
  re-clone `https://gitlab.synchro.net/main/sbbs.git`, sparse `src/conio`,
  `src/syncterm`. TTYPE reply is in `src/syncterm/telnet_io.c:226`.
- amiexpress-web: `web/backend/src/server/telnet-server.ts` (TTYPE promise,
  `TTYPE_GRACE_MS` / `TTYPE_PROMISE_MAX_MS`), test
  `web/backend/tests/transport/ttype-promised-late.test.ts`, log
  `logs/backend.log`.

## Recent Changes (this session, in order)

Defects found and fixed, each with a test where the logic is host-testable:

1. `DCTelnet.c` 8-bit bytes corrupted AGAIN by an earlier commit (46 U+FFFD,
   incl. live CSI `0x9B` strings: garbage in disconnect/lookup/speed-test
   messages). Restored from upstream bytes; `make check-encoding` guards it.
2. PETSCII CR (13) and shifted CR (141) emitted bare CR -> every line
   overwrote the previous one. Now CR+LF.
3. Output buffer could truncate expanded output. Now `PETSCII_MAX_OUT_PER_BYTE`
   (17) and chunked translation in `Receive()`; test checks all 256 bytes.
4. `g_petsciiState` only initialised by the menu toggle -> with the mode saved
   in prefs, `cols = 0` and a CR+LF after every glyph. Now reset per connection.
5. Toggle called `SetFont()` on the live window; now the `shouldRestart` path
   like Settings > Font, and `OpenAppScreen()` opens/`CloseDisplay()` closes
   the PETSCII fonts.
6. Fonts only searched in `FONTS:`; now also `PROGDIR:Fonts/`, with a notice.
7. **Fonts never loaded at all**: the generator emitted an extra `dc.l 0`
   before the `moveq/rts` stub, so `DiskFontHeader` sat 4 bytes late and
   `OpenDiskFont()` failed. Also `FPF_DESIGNED`, the default-glyph CharLoc
   entry, CharLoc last. Every earlier hardware round ran the CP437 fallback.
8. amiexpress-web showed its first screen 500 ms after connect regardless of
   TTYPE; a late "PETSCII" could not undo the ANSI front door (log proof:
   `TELNET-FRONT` display before `terminal type: "PETSCII"`). Server now waits
   for a promised TTYPE (peer sent WILL) up to 3 s.
9. ibmcon has no SGR 27 / SGR 22 (ANSI.SYS subset): reverse never ended
   (logo solid white), bold leaked. Attributes are now rebuilt from SGR 0 +
   colour + SGR 7 on every change.
10. ibmcon wraps immediately after the last column; with the 16-px font the
    window is 40 columns, so the console AND the translator wrapped -> every
    second line black. `PETSCII_CONSOLE_SETUP` (`CSI ?7l`) written after the
    console opens (after `isAppIconified` is cleared -- `ConWrite()` drops
    output while it is set).

## Repository loss (2026-09-24 15:16)

`~/Code/dctelnet-petscii` was replaced by an extracted release folder
(`DCTelnet/`, `DCTelnet.info`, `FILE_ID.DIZ`): macOS is case-insensitive, so
`~/Code/DCTelnet-PETSCII` IS the repo path. No remote, Trash or snapshot
existed; the 26-commit history is gone. Reconstructed on upstream `c244bc6`
from `~/Desktop/DCTelnet-PETSCII/src` + fonts + tools/tests rewritten from the
session transcript; the rebuilt tree reproduced the shipped binary byte for
byte except the build-hash strings. Commit `90b9efd` records this.

## Learnings

- **Check the rig before believing a hardware report.** One round tested an
  old binary because FS-UAE mounts `~/Downloads/DCTelnet-PETSCII`, not the
  Desktop package. Read the FS-UAE temp config (`hard_drive_N`) and the
  binary's build string.
- **Read the console's source before guessing its behaviour.** Two wrong
  hypotheses (SGR 27 support, screen-font line height) were settled in minutes
  by `/Users/spot/Code/ibmcon`: ANSI.SYS subset, immediate wrap, line height
  from the window RastPort font.
- **A race is proven from the server log's ORDER, not from one success.**
  Round 1 "worked" only because that connection won the 500 ms race. A delayed
  -reply probe (`ttype_race.py`-style Python client) reproduces it locally.
- **SyncTerm does nothing special for TTYPE** -- same `IS "PETSCII"` on SEND;
  it just answers fast. CGTerm negotiates nothing (board uses DEL-probe).
- **Verify a font the way the loader reads it**, against the known-good fonts
  in the archive, not against the struct definitions alone.
- **Latin-1 files**: `DCTelnet.c` / `DCTelnet-protocol.h` must be edited with a
  latin-1 Python read/replace/write, never a UTF-8 editor; `grep -a` on them.
- **`gh` is logged in as `johanBMS` by default**; create/fork/PR as `spotUP`
  with `gh auth switch -u spotUP`, then switch back. Git over SSH is spotUP.
- **Never keep the only copy of a repo without a remote**, and never give a
  release folder the repo's name (case-insensitively) next to it.

## Artifacts

- PR #14: https://github.com/bruno-frederic/dctelnet/pull/14 (description in
  `thoughts/shared/prs/14_petscii-mode.md`).
- Private repo: https://github.com/spotUP/dctelnet-petscii
- Public fork: https://github.com/spotUP/dctelnet (branch `feature/petscii-mode`)
- amiexpress-web commit `e25b8edd7` (live).
- `~/Desktop/DCTelnet-PETSCII/DCTelnet-PETSCII.lha` (build `167bbde`).
- Spec/plan: `docs/superpowers/specs/2026-09-24-dctelnet-petscii.md`,
  `docs/superpowers/plans/2026-09-24-petscii-emulation.md`.

## Next Steps (ordered)

1. Watch PR #14; address Bruno's review on `feature/petscii-mode` (push to
   `fork`). Open decisions are his: LGPL tables in an unlicensed codebase, and
   the fonts' unresolved license (fonts are optional; dropping them is safe).
2. Mirror any PR-branch change onto `feature/petscii-emulation` (the PR branch
   has scrubbed comments in `petscii_dispatch.h`, `petscii_keymap.h`,
   `Fonts/README.txt` that the private branch does not).
3. Hardware-verify the upper/lower charset swap (14/142) -- never observed.
4. Not built: RUN/STOP mapping (no natural Amiga key chosen), INSERT (148),
   cursor-left/right wrap at the screen edge, C64 blue background / light-blue
   default text.
5. Optional separate upstream PR: make `src/aos68k` work off Windows (it uses
   `%%VBCC%%` placeholders and `del`).
6. Backlog from the original spec: WiFi-modem serial connectivity, other
   emulations (ATASCII etc.), native AGA/RTG display backend.

## Other Notes

- The `/opt/homebrew/opt/vbcc/NDK3.2R4/Include_H` symlink points into the
  destroyed worktree and is dangling; the local `aos68k` bypasses it.
- amiexpress-web's root `handoff.md` was not updated for `e25b8edd7`.
- The PR body says "Tested on an emulated A1200 (FS-UAE)"; no real-hardware
  test of this build has happened.
