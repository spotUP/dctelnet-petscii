---
date: 2026-11-12
topic: Session end -- LiveChat picture/menu work handed to amiexpress-web-02; DCTelnet open items
status: open
---
# Session end: LiveChat picture work handed to amiexpress-web-02

The owner stopped this DCTelnet session on 2026-11-12 and asked that all of its work go to the
amiexpress-web session (amiexpress-web-02). Read thoughts/spot/todos.md rows dated 2026-11-12 first;
trust the ledger over this summary. Previous handoff: 2026-11-12_kitty-upstream-86-music-open.md.

## Closed this session (owner-confirmed)
- ANSI music in bulletin 56: PASS. Cause: rig 3 was muted (vtcon tools/rig/rig.py rig_volume() is 100
  only with `start --sound`). No code change.
- IMAGE-LOGO-DITHER: PASS ("the blue logo is perfect"), vtcon 5fe845d6.
- LiveChat scrolling: PASS on board e4c20bd92.
- Fork PR spotUP/dctelnet#13 merged as 43266a8.
- LIVECHAT-FOCUS-PIC (picture vanished on chat log border click / Tab): DOOR cause, not DCTelnet. The
  TS door sent a=d when the chat panel got focus. Board fix 0155caabb; owner PASS for click and Tab.
- LiveChat picture lifted while menus are open: owner chose "never lift". Board 6b18bde04; owner
  confirmed the picture stays while menus are open.

## Open, owned by amiexpress-web-02 from now on
1. LIVECHAT-MENU-PIC: with 6b18bde04 the menus draw behind the picture. A z<0 placement alone does not
   help, because the door paints menus with default background (SGR 49). Fix: board 8e6e326b9 (pushed to
   origin/main, deploy was in progress at session end; live /health still showed 6b18bde04). While an
   overlay covers part of the picture, the door cuts the picture into up to 8 cropped p= placements around
   the overlay at z=-1073741825. On close it deletes the pieces and places it whole again.
   - Client support checked by code read only (vtcon main 5f5a7061), not yet on the Amiga:
     source crop x,y,w,h in kit_show (engine/kitty.c:1038); same i+p replaces one placement
     (kitty.c:1098-1103); a=d,d=i,i=N,p=M deletes one piece (kitty.c:1296); 64 image slots
     (VT_IMG_SLOTS, vtengine.c:142); z<0 draws text over the picture; DCTelnet's opaque cover skip
     (fd342ee) excludes z<0 pictures.
   - Seam risk: crops must be cut on cell boundaries in source pixels, and each piece's c/r must equal
     its cell count, or 1-pixel lines show at piece edges.
   - NEXT: when https://bbs.uprough.net/health shows 8e6e326b9, ask the owner to retest LIVE in DCTelnet
     on rig 3: open LiveChat, show a picture, open a menu. PASS = menu text fully visible, picture
     shown around the menu, whole again after closing. If pieces fail to draw or leave seams on the
     Amiga, that is a DCTelnet/vtcon issue: tell the owner and up-term-ba (vtcon) -- do not edit vtcon.
2. LIVECHAT68: the same change is built but not installed on the board.

## Rig 3 (owner's test rig)
- Started with `UPTERM_RIG=3 python3 tools/rig/rig.py start --sound --purpose "DCTelnet session: LiveChat
  menu keeps picture retest (board 6b18bde04)"` from /Users/spot/Code/UP-Term/vtcon, held 4 h from about
  01:30Z (`rig.py hold --release` or `rig.py stop` ends it). The window title still names this DCTelnet
  session; restart with a new --purpose if the task changes.
- DCTelnet binary on the rig: vtc/dct/DCTelnet.020, md5 49e8688107c36c5c496e252475a48a96 = build
  1c23f50. No client rebuild is needed for the LiveChat retest.
- Never touch rigs 1, 2 or 68.

## Left for the owner (not handed over: DCTelnet repo, not the board's)
- Worktree /Users/spot/Code/DCTelnet/dctelnet-focuspic (branch fix/livechat-focus-picture, from fork/main
  43266a8) holds an uncommitted host test test_kfp_the_door_focus_clicks_leave_its_picture_shown and a
  harness fix (Text() mock calls[8192] overflow). No client bug exists, so the test is optional; the
  harness fix could be its own commit. Remove the worktree when the owner says.
- Docker data 3.9 GB under ~/Library/Containers/com.docker.docker was not pruned (Docker not running).
  Owner approved pruning unused images and build cache once Docker is up.
- Disk: cleanup on 2026-11-12 took / from 2.5 GB to 10 GB free (seven clean merged DCTelnet worktrees,
  amiexpress-web agent-a39fc0385 worktree, vtcon build/rig-saved/69 and dist-full, ~/Library/Caches).
- KITTY-SCROLL-SPEED: up-term-ba (vtcon engine/kitty.c), queued after vtcon v1.1; it writes when a
  commit exists.
- Upstream bruno-frederic/dctelnet#86 open, head a363b79. The RIP speed fix goes upstream only with RIP.
