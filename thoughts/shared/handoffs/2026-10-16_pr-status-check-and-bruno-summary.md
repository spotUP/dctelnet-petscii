---
date: 2026-10-16
topic: Status check of the upstream PRs; summary message drafted for Bruno
tags: [handoff, upstream-prs, toolbar, ssh, vtcon]
status: final
---

# Handoff: PR status check, summary for Bruno

## Task(s)

- Resumed from `2026-10-16_vtcon-engine-and-open-prs.md` and checked the upstream PRs for
  new review comments. **Done: nothing new.** The last comment in every thread is ours.
- Drafted a plain-text summary for the owner to paste to Bruno (Discord DM, AmigaDev server,
  user `_Bruno_`). **Status: handed to the owner. Not confirmed sent.**

## State (2026-10-16)

- 20 upstream PRs are open (#27-#44, #47, #52, #56). None has a review decision.
- **#27 (tool bar icons)**: our last comment (2026-10-16 14:21) asks whether the 11 s
  WinUAE timing used 5be62b1 or the earlier build. It also asks for two A500 timings with
  5be62b1: the tool bar on, and the tool bar off on its own. Bruno has the real A500 next week.
- **#42 (SSH)**: our reply (2026-10-16 10:53) says c36406d fixes the `-Werror` comparison
  failure in walkero/docker4amigavbcc:m68k-amigaos-2.0.0. Bruno has not confirmed yet.
- **#47**: only our FS-UAE test note. No review.
- **#56 (vtcon engine)**: no comments yet. It is stacked on #52.

## Message drafted for Bruno (as given to the owner)

Covers: the #27 question and the two timings, the #42 fix in c36406d, what #56 is
(stacked on #52), and that the other PRs are unchanged and waiting for review.

## Recent Changes

None in code. This file is the only change.

## Learnings

- The working tree on `feature/petscii-emulation` has build output that is not committed:
  `src/aos68k` (the submodule is modified), test binaries in `test/`, and
  `ibmcon/ibmcon.device`. Leave them uncommitted. Do not clean them without the owner's approval.
- Check PR activity with:
  `gh pr list -R bruno-frederic/dctelnet --author spotUP --json number,updatedAt,comments`
  then `gh pr view <n> --json comments` and `gh api .../pulls/<n>/comments` for the
  threads that changed.

## Next Steps (in order)

1. When Bruno replies: answer on the PR. Fix in the owning branch, then replay with the
   workflow in `2026-09-28_v2-stack-submitted.md` (`repick.sh`, `buildall.sh`, push with
   a lease). `feature/pr-vtcon-engine` is the top of the stack.
2. #27: if the timings with 5be62b1 are still slow, profile the remap on the emulator.
3. Owner items in `thoughts/spot/todos.md`: U4, S3.5b (bbs.uprough.net:31337), and the
   #11 live-BBS follow-up.
4. Cleanup after merge: see `2026-10-16_vtcon-engine-and-open-prs.md` step 4. Delete the
   `dctelnet-vtcon` worktree and `feature/vtcon-engine` only with the owner's approval.

## Other Notes

The previous handoff `2026-10-16_vtcon-engine-and-open-prs.md` stays valid for the #56
verification and the smoke rig details.
