---
date: 2026-11-02
topic: Docker-free build pushed to release/2.0-UP-X; release archive cb63c78 built
tags: [handoff, nodocker, release, 2.0-UP-X]
status: final
---

# Handoff: Docker-free build pushed, release archive cb63c78 on the Desktop

## State

- `spotUP/dctelnet` branch `release/2.0-UP-X` is at `cb63c78` (fast-forward from
  `6f3e17a`, verified with `git ls-remote`). New on the branch:
  - `3751042` Rebuild test helpers when the host changes (tests/Makefile host-id).
  - `cb63c78` Build the CI package without Docker (`make host-ci`, `build/host-toolchain.sh`).
- Integration worktree `/Users/spot/Code/DCTelnet/dctelnet-integration`, branch
  `local/integration-2026-10-31`, equals the remote branch. Clean apart from an
  untracked `tools/__pycache__/` file.
- Release archive: `~/Desktop/DCTelnet-2.0-UP-X-cb63c78.lha`, md5
  `b8028364b08e87df5b7a614e06c4a47b`, 95 files, version string
  `DCTelnet 2.0-UP/X (build cb63c78) (Nov 02 2026)`. Built with
  `cd src && nice -n 19 make host-ci AMIGADATE=2.11.2026` (exit 0).

## How to build now

- No Docker. Toolchain lives in `~/Library/Caches/dctelnet-toolchain` (vbcc 0.9hP3,
  same as the old image; host output was verified equal to Docker except build
  hash and `__DATE__`). Rebuild the toolchain with `build/host-toolchain.sh`.
- `make ci` (Docker) stays the upstream-authoritative check per AGENTS.md; the
  builder image is deleted locally.

## Not done / open

- The cb63c78 archive has not run on hardware. Not copied to the NAS drop
  (`/Volumes/home/DCTelnet-<commit>.lha`) or the A1200.
- `~/Desktop/DCTelnet-2.0-UP-X/` is a stale unpacked copy from 2026-11-01 23:49.
- Open ledger rows in `thoughts/spot/todos-upstream-prs.md` (git-ignored ledger):
  SSH connect to non-SSH server gives no message (line ~421), U+279C prompt arrow
  glyph (~389), SSH TERM case (~346), ANSI music sync with UP-Term/DEViLBOX (F),
  AMIBERRY-DEFAULT (belongs to a UP-Term session).

## Gotchas

- Start sessions from `dctelnet-petscii-recovered`, not `~/Code/DCTelnet`: the
  `go` hook only looks in the current directory's `thoughts/`.
- Stale unused slot: `dctelnet-vtcon/thoughts/spot/next-session-dctelnet.md`
  (2026-10-30, MA1). Delete or use it.
