---
date: 2026-11-11
topic: Bulletin 90 leaves the terminal at half height - root cause and fix
status: fixed in int87 worktree (86f6a33), verified on rig 4, not pushed
---

# Bulletin 90: half-height screen after the RIP bulletin

## Root cause

Layer: DCTelnet front end, `src/vtbuiltin.c` (worktree `dctelnet-int87`). Not the vtcon engine.

Bulletin 90 sends `!|w000D270O10`, a RIP text window on rows 13-24. The engine (`rip_window` in
`src/third_party/vtcon/engine/rip_vt.c`) turns that into a scroll region with origin mode. In RIP Auto
mode DCTelnet leaves RIP when a write carries ESC[2J and no RIP command follows. In build 8090683 that
decision was made by `VtRipAutoLeave()`, called from `VtWriteIn` at the END of the write, after `VtFeed`
(8090683 `src/vtbuiltin.c:1848` and `:1861`). The board's door frame is one write:
ESC[0m, colour, ESC[2J ESC[H, then the whole screen (`amiexpress-web/sdk/c/src/ui_ansi.c:121-129`,
called from `sdk/c/src/menudoor.c:648` and `:1577`). So every byte after the clear was already laid out
inside the bulletin's text window before the window was reset. The reset then came too late: the text
sat in rows 13-24 and the top half stayed blank.

Violated invariant: a clear that ends RIP must end it before the bytes after the clear are laid out.

## Why the two earlier fixes missed it

1. The owner's retest did not run the fix. Rig 3's binary
   `/Users/spot/Code/UP-Term/vtcon/build/rig3/vtc/dct/DCTelnet.020` carries the version string
   "build 241128d" (md5 699befa10117aba30ad72b2b08e2396c). No build-8090683 binary was on rig 3 or rig 68.
2. Build 8090683 itself still fails on the door path. Its test (`test_rip_bull_5_...`, pass 2) only
   checked that an ESC[H in a LATER, separate write reached row 0. It never checked where the text of
   the clearing write itself landed, and it was never run on a rig.
3. The `!|*` reset the board sends after a direct `B 90` hid the bug on that path: with `!|*` the engine
   resets the window by itself. The bulletins door sends no `!|*`.

## Fix

Commit 86f6a33 in `/Users/spot/Code/DCTelnet/dctelnet-int87` (branch local/int87-2026-11-10), not pushed.
No change in the vtcon submodule.

- `VtClsScan` (`src/vtbuiltin.c:1692`) returns the byte count up to and including the J of ESC[2J.
- `VtFeedRip` (`:1768`) cuts the slice at the clear. In Auto mode with RIP active and no RIP introducer
  in the rest of the write (`VtRipFollows`, `:1717`) it calls `VtRipLeave` (`:1727`) at once and feeds
  the rest as plain text through `VtFeedText` (`:1850`, the Auto loop taken out of `VtFeed`).
- A write that may still bring RIP keeps the old end-of-write rule (`VtRipAutoLeave`, `:1734`).
- Sentinel `vtRipClsLeaves` counts the early leaves.

## Test

`test_rip_bull_6_the_screen_after_the_clear_gets_all_rows` in `tests/test_term_engine.c:2741`. It drives
`VtWriteIn` (through `vt_put`) with bulletin 90, the prompt, then the door frame in one write and split
inside the clear sequence. It asserts the sentinel, RIP inactive and the cursor at row 0 col 16. A third
part checks that a clear followed by RIP on a new line keeps RIP.

Fail before (fix reverted, sentinel stubbed):
`Assertion failed: (vtRipClsLeaves == leaves + 1), function test_rip_bull_6_the_screen_after_the_clear_gets_all_rows, file test_term_engine.c, line 2766.`

Pass after: `term_engine: all assertions passed` (rc 0).

Note: the engine accepts `!|` only at the start of a line. "ESC[2J ESC[H !|*" on one line is text, so
RIP leaves there too. `VtRipFollows` is the cautious side of that: it keeps the old rule when unsure.

## Rig result (rig 4, replay of the captured wire, 640x256 screen, 32 text rows)

Replay server: bulletin 90 from the wire capture, then a 30-line menu, then 30 more lines without clear.

| Binary | Case | Result |
|---|---|---|
| build 241128d (rig 3's) | `!|*` + ESC[2J menu | full height |
| build 8090683 | no `!|*`, ESC[2J menu (door path) | FAIL: only lines 20-30 in rows 13-24, top half blank |
| build 86f6a33 | no `!|*`, ESC[2J menu (door path) | PASS: lines 01-30 + prompt on rows 0-30; next 30 lines scroll over all rows |
| build 86f6a33 | `!|*` + ESC[2J menu | PASS: screenshots byte-identical to the row above |

## Open

- Rig 3 still holds build 241128d. New binary:
  `/Users/spot/Code/DCTelnet/dctelnet-int87/build/vbcc-68020-release/DCTelnet.68020` (build 86f6a33).
- A screen after the bulletin with neither `!|*` nor ESC[2J cannot be recovered by the terminal: the
  text window is what the board asked for. That case needs the board-side reset (amiexpress-web-02).
  Not run on the rig with the new build (ports 4001 and 4003 of the replay).
- Whether the live main menu starts with a clear: not determined.
- One host test run started while another session's vitest worker was up (a debug run, a few seconds).
