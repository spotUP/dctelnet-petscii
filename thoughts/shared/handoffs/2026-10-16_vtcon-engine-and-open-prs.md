---
date: 2026-10-16
topic: vtcon engine submitted as #56; 20 upstream PRs open, all waiting on review
tags: [handoff, vtcon, upstream-prs, toolbar]
status: final
---

# Handoff: vtcon engine submitted, the stack waits on review

## State

Nothing on our side is open. Everything below waits on Bruno or on the owner.

- **20 upstream PRs open**: #27-#44, #47, #52, #56
  (`gh pr list -R bruno-frederic/dctelnet --author spotUP`). #14-#26 are merged or closed.
- **#56 (new, 2026-10-16)**: built-in renderer, where vtcon's engine parses and
  retro32-term draws. Fork branch `feature/pr-vtcon-engine`, stacked on #52; only the last
  five commits are its own. It was rebuilt from `feature/vtcon-engine` by cherry-picking the
  vtcon commits onto `fork/feature/pr-petscii-case-switch` (2448df3). This dropped the
  LICENSE commit/revert pair and the duplicated PETSCII commits. The tree is identical to
  `feature/vtcon-engine` f7cb4c2.
- **#27 (tool bar icons)**: fork `feature/pr-wb-toolbar-icons` at 5be62b1. Reply posted:
  https://github.com/bruno-frederic/dctelnet/pull/27#issuecomment-6214982370. Waiting on
  Bruno to time the tool bar on and off on his A500.

## Verification done for #56

- `make ci` from `src/` passes (vbcc, 68000 and 68020, warnings as errors). On macOS it needs
  `make ci DOCKER_USER=$(id -u):$(id -g)`: the Makefile gets the user with GNU
  `stat --format`, which fails on BSD stat and leaves `--user` empty
  (`docker: unable to find user -q`).
- `cd tests && make` passes.
- Emulated A1200 (FS-UAE), built-in renderer, 68020 build, with a host telnet stream.
  Cursor position, SGR colours, `CSI I` and the DSR reply (`ESC[24;5R`) all PASS.
  Box bytes kept their cells. The glyphs showed as Latin-1 because the run had no prefs
  file and used ROM topaz. `src/vtbuiltin.c:51` maps CP437 bytes to the same font index,
  as `term_feed` did, so this is not a regression. Not re-run with the CP437 font.
- Smoke rig (scratchpad, temporary):
  `/private/tmp/claude-501/-Users-spot-Code-DCTelnet-dctelnet-vtcon/d3a14cdb-85a1-4f83-930c-b71c6eb7070b/scratchpad/vtsmoke/`
  (srv.py, host.log, host.png, am/t.fs-uae).

## Learnings

- `ami.py shot` decodes a depth-4 DCTelnet screen as colour noise. Take a host capture of
  the FS-UAE window instead.
- ARexx `CONNECT` takes `CONNECT host port` with a space. `host,port` fails silently.
- No test measures how fast the tool bar opens (#27). Nothing fails if the remap gets slow.
- `.gitmodules` points retro32-term at `../retro32-term.git`. From Bruno's repo this
  resolves to bruno-frederic/retro32-term, which lacks 2aec2db until he merges
  retro32-term#1 (affects #43 onward, not new in #56).
- `feature/vtcon-engine` is not based on `feature/petscii-emulation` and has no
  `thoughts/` tree. Handoffs stay on `feature/petscii-emulation`.

## Next steps (in order)

1. Answer review comments as they arrive. Fix in the owning branch, then replay and push
   with the workflow in `2026-09-28_v2-stack-submitted.md` (`repick.sh`, `buildall.sh`,
   push with a lease). Add `feature/pr-vtcon-engine` to the repick list as the top of the
   stack.
2. After Bruno times #27: if the tool bar is still slow, profile the remap on the emulator.
3. Owner items still open in `thoughts/spot/todos.md`: U4 (email contact with the author),
   S3.5b (owner test against bbs.uprough.net:31337), the #11 live-BBS follow-up.
4. Cleanup once merged (list in the 2026-09-28 handoff), plus the `dctelnet-vtcon`
   worktree and `feature/vtcon-engine` (local, `origin`, `fork`; superseded by
   `feature/pr-vtcon-engine`, delete only with the owner's approval).
