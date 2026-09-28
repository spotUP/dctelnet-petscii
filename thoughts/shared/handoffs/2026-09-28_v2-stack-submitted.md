---
date: 2026-09-28
topic: "DCTelnet: the PR stack ported onto v2.0-dev and submitted (#15-#47, retro32-term#1)"
tags: [dctelnet, v2.0-dev, prs, handoff, ssh, ibmcon, retro32-term]
status: final
---

# Handoff: v2.0-dev stack submitted, waiting on Bruno's review

Follows `2026-09-28_v2-port-and-builtin-renderer.md`, which covers the port
itself and the built-in renderer work.

## Tasks

- Done: the owner asked "we should submit our PRs on top of his 2.0". Each part of it
  was a decision the owner made:
  - update #15-#40 in place;
  - close #14 with a note;
  - fork retro32-term and open a PR there.
- Done: the review pass before pushing, which is §9 of the rules. Every finding marked
  FIX is fixed in the branch where it entered, and the stack was replayed.
- Done: follow-ups the owner asked for during testing:
  - #45, the PETSCII fonts: the rendered fonts stay and the generator goes;
  - #46, ibmcon.device 1.11 in the package;
  - #47, the Login Macro help.
- Done: SSH as `spot` to uPt0wN disconnected after the user name was entered. The
  cause was a second login of the same user, which AmiExpress refuses. It was not a
  DCTelnet bug. The owner's retest works.
- Owner-verified: #47's dialog on build 8ab9e2e.
- Open: the upstream review of #15-#47 and retro32-term#1. Cleanup after they merge.

## Critical references

- Upstream is bruno-frederic/dctelnet, base `v2.0-dev`. The fork is spotUP/dctelnet
  (remote `fork` in `~/Code/dctelnet-v2`).
- Mapping from port branch to PR head to PR number:
  - `thoughts/spot/v2tools/push_map.txt` (branch to head);
  - `v2tools/gen.py` STACK (numbers #15-#40);
  - `v2tools/numbers.txt` (#41 prefs-growth, #42 ssh, #43 builtin-layers,
    #44 console-fixes).
- PRs outside the linear stack:

  | PR | Head | Commit | Base |
  |---|---|---|---|
  | #45 | `feature/pr-petscii-fonts` | 759e950 | directly on v2.0-dev |
  | #46 | `feature/pr-ibmcon-device` | 5c33494 | stacked on #30 |
  | #47 | `feature/pr-macro-help` | 8ab9e2e | stacked on #44 |

- retro32-term: bruno-frederic/retro32-term#1, from spotUP:feat/layer-aware (2aec2db)
  into `feat/dctelnet-integration`. DCTelnet's `src/third_party/retro32-term`
  submodule points at 2aec2db from #43 on; on the branches before #43 it points at
  b291015.
- The PR bodies as posted are in `thoughts/shared/prs/v2/<n>_<key>.md`, plus
  `14_closed-note.md` and `retro32-term_1_layer-aware.md`.
- Where to work:
  - worktree `~/Code/dctelnet-v2` (branches `port/*` and `feature/pr-*`);
  - ledger `thoughts/spot/todos.md` (section S1-S6 and after).

## Recent changes (this session)

- Review findings: `v2tools/review_findings.md` has the list and the triage. Notable fixes:
  - R6.3 (SSH): only key-exchange messages count before the first NEWKEYS.
    Authentication messages count only while logging in, channel messages only once
    the channel is open. `test_nothing_before_the_keys` fails before the fix.
  - R6.4: KnownHosts is read whole (`ReadWholeFile`, 256 KB). It is rewritten only
    from the whole file.
  - R4.7: the timer clock is the E-clock (`Ticks_FromEClock`, host-tested).
  - R3.7: ESC 8 keeps DECCKM and iCE. Rig: pre-fix `ESC [ A`, fixed `ESC O A`.
  - R4.1: ibmcon BlinkTick keeps its count across ToggleCursor.
  - R4.2: a PETSCII paste turns each line end into one Return.
  - R4.3: a clip over 16 KB pastes its start instead of nothing.
  - R4.4: the selection is reset when the display closes.
  - R4.5: the Capture check mark survives a reopen.
  - R4.6: `lastConnectErrno` is reset per connect.
  - R5.1: timer and ARexx signals are waited on while iconified.
  - R5.2: a macro wait cancelled by a disconnect prints no message.
  - R5.5: the local echo decodes UTF-8.
  - R5.7: a bare `ESC[M` is Delete Line, not music (test fails before the fix).
  - R6.1: import stops when the Address Book exists but is unreadable (new
    `FileLength()` in utils).
  - R6.6: the built-in renderer's NAWS is the engine's grid (80, not width/8).
  - R6.7: the engine comments (retro32-term 2aec2db).
  - R3.5: the wb-toolbar-icons ObtainPen now uses ansiColourPens.
- Not fixed, disclosed: R1.3, R1.4, R1.9, R3.8, R4.8, and R1.5/R1.6 ("Known limits"
  in #16's body).
- Verification of the tip (79ce5d7 before #47):
  - all 30 branches build with vbcc (68000 + 68020, warnings as errors) and pass
    `tests/` (`v2tools/rig/buildall.txt`);
  - the drop audit is unchanged;
  - rig checks RV.T1-T6 pass (`v2tools/rig/results.json`): E-clock WAITFOR, SSH with
    rekey, NAWS on a 1280-wide screen, DECCKM over ESC 7/8, blink, own-screen
    backfill.
- Pushes: force-pushed with leases, which are the fork heads saved before
  (`v2tools/fork_heads_before.txt`). Backup branches `backup-2026-09-28-*` are in
  dctelnet-v2. All 33 PRs were MERGEABLE at submit (`v2tools/prs_after.json`).
- #14 closed. Its last commit (2a584f6) never reached v2.0-dev and was resent as #45.
- The owner's test drawer `~/Downloads/DCTelnet-PETSCII/DCTelnet-v2/` holds:
  - build 8ab9e2e (the #47 tip, 68000 + 020);
  - ibmcon 1.11 in Devs/.
  All three were compared identical after the copy.

## Learnings

- The retro32-term submodule does not follow `git switch`. Check out each branch's
  gitlink before building (`buildall.sh` does:
  `git -C src/third_party/retro32-term checkout $(git ls-tree HEAD ... )`).
- A device test must `Avail FLUSH` first. ibmcon stays resident after DCTelnet quits,
  so a swapped `Devs/ibmcon.device` was not loaded. The first "pre-fix" run tested the
  fixed device.
- The package shipped ibmcon **1.4** (1998), not 1.6. #46 fixes this, and its
  `tests/` target `check-ibmcon` fails when the package and the source build differ.
- zsh takes `$key:r` as a modifier: `port/$key:refs/...` pushed nonsense. Write
  `${key}`.
- `.gitattributes` makes everything under `build/package/` binary. Text added there
  (Petscii.readme) does not show in PR diffs, so paste it into the body.
- `GTTX_Justification` (not `GTTX_Justify`), `GTJ_RIGHT`, V39.
- `repick.sh BRANCH` replays every later port branch after an amend. `guide_resolve.py`
  merges the recurring feature-list conflict in the guide. The copies in `v2tools/`
  still use the session's scratchpad paths: fix `D=`/`S=` before reuse.
- The rig drawer (`repro/`, 1.5 GB) lived in the session scratchpad and is gone with
  it. Rebuild notes:
  - `v2tools/rig/repro.fs-uae` and `boot-go`;
  - `amiagent/`;
  - `setup.py` writes the DCTelnet drawer;
  - boot with `open -g -n -a /Applications/FS-UAE.app --args <cfg>`;
  - kill only by config path.

## Artifacts

- Docs on `feature/petscii-emulation` (local), in time order:
  - 860c1d3: PR bodies;
  - 508b629: #45;
  - 0c1cc5e: #46;
  - 139fb40: the 1.4 correction;
  - 7a8d97f: #47;
  - this handoff.
- `thoughts/spot/v2tools/`: the scripts and records named above. It is gitignored and
  stays on disk.

## Next steps (in order)

1. Watch the review comments on #15-#47 and retro32-term#1 (`gh pr list -R
   bruno-frederic/dctelnet --author spotUP`). Answer and fix in the owning branch. Then
   replay with `repick.sh`, rebuild with `buildall.sh`, and push with a lease on the
   current fork head.
2. After each upstream merge, bring `v2.0-dev` into the next branch before touching it.
   #46 can merge any time after #30; #45 at any time.
3. If Bruno merges retro32-term#1, check whether #43's gitlink should point at his
   merge commit rather than at 2aec2db.
4. Cleanup (V34), once merged and not before:
   - `git worktree remove ~/Code/dctelnet-v2`;
   - delete `port/*`, `feature/pr-*` (local and fork) and `backup-2026-09-28-*`;
   - check each branch is merged or superseded first.

## Other notes

- `src/smakefile` has the SSH crypto objects but was never built (there is no SAS/C
  here).
- Open, minor: a menu opened by another task between `BuiltinCovered()` and a write is
  a race (from the previous handoff).
