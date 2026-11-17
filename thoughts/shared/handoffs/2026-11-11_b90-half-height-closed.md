---
date: 2026-11-11
topic: B 90 half-height after a RIP bulletin -- closed
status: closed
---
# B 90 half-height after a RIP bulletin: closed

Owner retested LIVE on bbs.uprough.net on 2026-11-11 and reported "pass finally".

## What fixed it
- DCTelnet (int87 worktree /Users/spot/Code/DCTelnet/dctelnet-int87, branch local/int87-2026-11-10, not pushed):
  - 86f6a33 "RIP Auto: leave RIP at the clear, before the rest of the write is laid out". RIP Auto left RIP
    only at the end of a write, so the bulletins door's frame (ESC[2J ESC[H plus the menu, one write) was laid
    out inside bulletin 90's |w text window. Test: tests/test_term_engine.c:2741. Root cause write-up:
    thoughts/shared/research/2026-11-11-b90-half-height-root-cause.md.
  - 703e259: vtcon submodule moved to branch local/dct-on-main = vtcon origin/main b22ecbac (holds the three
    RIP commits, pushed by up-term-ba on owner order) plus six mouse/kitty commits not on main yet.
- Board (amiexpress-web, owned by session amiexpress-web-02): 590c58f48 -- owed !|* set by content, paid at
  door start. Live /health showed 590c58f48 before the retest.
- Why earlier fixes looked like failures: rig 3 ran a stale DCTelnet build (241128d).

## Still open
- The six mouse/kitty vtcon commits (347f934d b9910c83 9c5f3e81 76efdba8 2d665ee4 667f5e18 on
  local/dct-on-main): up-term-ba and the owner decide whether they go to vtcon main.
- int87 PR to the fork's main (never upstream) not opened.
- Pending owner retests: IMAGE-LOGO-DITHER, LIVECHAT68 kit_band scroll.
- Older ledger items: IMAGE-MOUSE-FLICKER, IMAGE-ARTIFACTS, RMB-MENU-DCT, RIP-COLOUR-16, RIP-LOGIN-OVERDRAW, RIP-SLOW.
- Rig 3 was started with --idle-minutes 0 --no-amarchy-block --no-solidwin; restore normal args at next restart.
- Board-side gap (amiexpress-web-02): a non-door screen after a RIP bulletin with no pause and no line input; none known.
