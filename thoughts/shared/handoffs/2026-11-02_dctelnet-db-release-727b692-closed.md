---
date: 2026-11-02
topic: dctelnet-db session end; v2.0-UP-X tag and asset on 727b692, upstream PRs #77-#83 open
tags: [handoff, release, 2.0-UP-X, upstream-prs, petscii]
status: final
---

# Handoff: dctelnet-db session end (2026-11-02)

## Task(s)

- PETSCII display fixes, device checks, upstream PR publishing, unofficial 2.0-UP/X
  pre-release refresh. All done. No open work in this session.

## State

- Fork `spotUP/dctelnet`: `release/2.0-UP-X` = `727b692`, tag `v2.0-UP-X` = `727b692`
  (moved by the owner 2026-11-02, ls-remote verified). Release `v2.0-UP-X` is a
  pre-release; asset `DCTelnet-2.0-UP-X.lha` is the `727b692` package
  (md5 11838b41d79929646c4f042053b09a96). Fork `main` carries the same README/docs commits.
- Local backup tags (not pushed): `backup-2026-11-02-v2.0-UP-X` = 6f3e17a,
  `backup-2026-11-02-v2.0-UP-X-ce6fba3` = ce6fba3. Delete when no longer wanted.
- Desktop package: `~/Desktop/DCTelnet-2.0-UP-X-727b692/` (DCTelnet.lha, DCTelnet-debug.lha, unpacked).
- Upstream `bruno-frederic/dctelnet`: our PRs #42-#83 open, all MERGEABLE (2026-11-02).
  New this session: #77 (CI package without Docker), #78 (check-encoding BSD grep),
  #79 / #80 (on #56), #81 (PETSCII mode change full reopen), #82 (C64 fonts built in,
  depends on #73), #83 (reopen keeps screen, branch `fix/reopen-keeps-screen` = eeadc75,
  depends on #56, bumps vtcon pin to 9716735e). #75 got 3fcfde3 (ANSI music).
- PR #56 head = `6da6668` (redraw port). The A1200 check of it is closed: owner said
  "uae checks are enough"; the same code (d9f66d7) passed on the A1200 in dbf03fe.

## Critical References

- Ledger: `thoughts/spot/todos-upstream-prs.md` from `## 2026-11-02 dctelnet-db` (line 533) to the end.
- Integration worktree: `/Users/spot/Code/DCTelnet/dctelnet-integration` (branch `release/2.0-UP-X`).
- Prior handoff: `thoughts/shared/handoffs/2026-11-02_nodocker-pushed-release-archive.md`.

## Learnings

- Owner rules (2026-11-02): Bruno merges upstream PRs and does official releases; we never
  merge or release upstream. Unofficial 2.0-UP/X material (tag, release, README notice,
  docs/CHANGES, RELEASE-NOTES, release branch) stays in spotUP/dctelnet only.
  `remote.upstream.pushurl=no_push` is set in the integration clone.
- Unofficial release = all bleeding-edge features incl. unmerged upstream PRs, built from
  the integration branch.
- Device checks: emulator (Amiberry, `--emu amiberry`, `--os 32`) is enough. Never gate on
  the A1200. OS 3.1 on Amiberry stalls at boot under host load (vtcon row AMIBERRY-RIG-HANG);
  the owner allowed fs-uae once for that.
- PETSCII fonts are now built into the binary (60752f2, src/petscii_glyphs.c); no Fonts/ drawer needed.
- Force-pushing a tag is blocked by the auto-mode classifier even with owner approval;
  the owner runs it with `!`.
- `make` with a DOCKER_RUN override on the command line needs `$$PATH`.

## Next Steps

1. Nothing pending. Wait for Bruno's review of #42-#83; answer review comments as they come.
2. If a PR merges upstream, rebase the dependent PRs (#79, #80, #83 on #56; #82 on #73).
3. Unverified for #82 and #83: Amiga cross build and a device check. Run on Amiberry
   if Bruno asks or before the next unofficial release.
4. Open curiosity (ledger): Address Book Connect never reached the server on the fs-uae
   rig 3 attempt; it worked on Amiberry (AM2). Investigate only if it recurs.

## Other Notes

- Next unofficial release: update docs/CHANGES, guide HISTORY node, release notes before packaging.
- This handoff file is not committed (branch feature/petscii-emulation has unrelated local changes).
