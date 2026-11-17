---
date: 2026-11-01
topic: Fold PETSCII Mode into Terminal Type (TERMTYPE_PETSCII)
status: approved, base #72 (pr-ssh-terminal)
---

# PETSCII as a Terminal Type

Owner approved 2026-11-01: the "PETSCII Mode" checkbox / menu toggle becomes the fourth Terminal Type
(Automatic / BBS / Unix / PETSCII). Own upstream PR.

## Facts (measured on dctelnet-integration 3202330)
- TERMTYPE_AUTO/BBS/UNIX: src/prefs.h:179-181, from commit 058a2db (upstream PR #56 feature/pr-vtcon-engine).
  Not on upstream v2.0-dev. #56 < #63 < #72 carry it.
- APP_PETSCII_MODE (prefs.h:158, bit 15) has ~36 uses: DCTelnet.c 22, guis.c 4, site_prefs.c 4,
  menu_checks.c 1, prefs_file.c 1 (1.x import), DCTelnet-protocol.h 1 (TTYPE), DCTelnet.h 1, site_prefs.h 1.
- PETSCII_SESSION() = connectionDisplay && STATE_IS(APP_PETSCII_MODE) (DCTelnet.c:295).
- SSH TERM call site: DCTelnet.c:1345 `PETSCII_SESSION() ? "PETSCII" : Vt_SshTermName(&prefs)`; only on #72's stack.
- Telnet TTYPE: DCTelnet-protocol.h:356 and DCTelnet.c:1357 special-case PETSCII the same way.
- Site overrides: TERMINAL_GROUP_FLAGS includes APP_PETSCII_MODE (site_prefs.c:37); TermType copied
  with the terminal group (site_prefs.c:65). Summary line site_prefs.c:420/427.
- Load clamp prefs.c:108 resets TermType > TERMTYPE_UNIX to AUTO.

## Design
1. prefs.h: `#define TERMTYPE_PETSCII 3`. One predicate `Prefs_IsPetscii(p)` (p->TermType == TERMTYPE_PETSCII);
   every STATE_IS(APP_PETSCII_MODE) / `State & APP_PETSCII_MODE` reads it. No second source of truth.
2. Bit 15 retired: kept as OLD flag, read only by migration. Load (prefs.c and each Book entry's terminal
   group): flag set -> TermType = PETSCII, clear flag. Clamp accepts up to TERMTYPE_PETSCII. Saving never
   writes the flag. Old DCTelnet reading new prefs: TermType 3 clamps to AUTO there (acceptable, note in PR).
3. Menu: drop "PETSCII Mode" item; Terminal Type submenu gains "PETSCII" (mutual-exclude masks over 4 items,
   MENU_TERMTYPE_PETSCII after UNIX so `- MENU_TERMTYPE_AUTO` mapping holds). menu_checks.c row moves.
4. Site prefs GUI: drop SG_TERM_PETSCII checkbox; Terminal Type cycle gains "PETSCII".
   Vt_CharsetAfterTermType: PETSCII leaves Charset alone (PETSCII has its own path).
5. vtpersonality.c: PETSCII case behaves as BBS for personality; Vt_SshTermName returns "PETSCII";
   call sites 1345/1357/protocol.h:356 drop their special case and use one name function.
6. site_prefs.c summary prints "PETSCII" from TermType.

## Tests (host)
- prefs migration: old file with bit 15 -> TermType PETSCII, bit clear; TermType 3 survives load.
- menu_checks: PETSCII item checked iff TermType PETSCII; old item gone.
- site_prefs: Book entry override with old flag migrates; summary text.
- vtpersonality: Vt_SshTermName PETSCII.
- Reachability: source check (tests/Makefile pattern, like check-ssh-terminal) that no APP_PETSCII_MODE read
  remains outside migration, plus one A1200 run: pick Terminal Type > PETSCII, connect to a PETSCII BBS,
  C64 font shows.

## Open (owner)
- DECIDED 2026-11-01: base #72 (pr-ssh-terminal); SSH special case dropped in this PR.
