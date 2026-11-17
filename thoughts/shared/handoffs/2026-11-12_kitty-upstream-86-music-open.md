---
date: 2026-11-12
topic: Kitty cover fix upstream on #86; ANSI music closed
status: superseded
---
# Kitty cover fix upstream on #86; ANSI music closed

Read thoughts/spot/todos.md rows dated 2026-11-12 first; trust the ledger.

## Done
- Upstream bruno-frederic/dctelnet#86 (fork branch feature/pr-kitty-graphics) head a363b79: fd342ee text-pass skip under
  opaque kitty pictures, a363b79 regression tests. PR body has an "Update" section. Port worktree removed.
- Fork PR spotUP/dctelnet#13 open (RIP batching + cover skip + test). Merge when the owner says.
- RIP speed PASS, RIP colours on 16 colours PASS (owner, rig 3, build 1c23f50).
- ANSI music in bulletin 56 PASS (owner, rig 3 restarted with --sound; the rig was muted, no code change).

## Open
- Waiting: up-term-ba reply on vtcon kitty scroll re-sample.
- Closed 2026-11-12: IMAGE-LOGO-DITHER PASS, LiveChat scrolling PASS (board e4c20bd92). PR #13 merged 43266a8, its worktrees removed.
- Open: LIVECHAT-FOCUS-PIC: door cause (a=d on chat-panel focus), board fix 0155caabb deploying. Owner retest when /health shows it. Worktree ../dctelnet-focuspic holds an uncommitted host test plus a harness Text() mock overflow fix; remove on PASS.
