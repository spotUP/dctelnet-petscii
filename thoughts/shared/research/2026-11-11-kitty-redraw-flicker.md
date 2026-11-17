---
topic: IMAGE-REDRAW-FLICKER (LiveChat door, picture flickers on every redraw with 1c23f50)
status: not reproduced on host -- DCTelnet's render path is clean for the door's byte shapes; cause outside the modelled stream
date: 2026-11-12
---
# Kitty picture flicker on every redraw (build 1c23f50)

## What the door sends (read from source)
web/backend/src/utils/kitty-graphics.util.ts + board/Doors/livechat/features/chat-pictures.ts:271-275:
- store: `ESC_G a=t,f=100,i=N,c=<cols>,r=<rows>,m=..;<b64 PNG>` (no s/v: PNG carries its size; no z; alpha only if the PNG has it)
- place: `ESC_G a=p,i=N,c=<cols>,r=<rows>,p=1[,x,y,w,h crop],C=1,q=2` (z=0, placement id 1 replaces the earlier placement, no a=d)

## Suspect (a): ruled out by code
- f=100 decodes to RGBA and goes through kit_make like f=32 (kitty.c:822-830). With c= and r= given, tw=c*cw, th=r*ch (kitty.c:1073-1076): stored at the display size, dh = h (vtengine.c:5725), so VtRowCover's dh test passes.
- Index 0 is transparent only: every palette path reserves pal[0] and maps only alpha < 128 to 0 (kit_quant kitty.c:100-106, exact palette kitty.c:~409-430, kit_adapt kitty.c:~381-385). A dark opaque photo has no index-0 pixel. fmt stays 0 unless vt_set_image_rgb is on (kitty.c:516; DCTelnet only turns on the RGB *copy*, vtbuiltin.c:1620/1638, which keeps fmt 0).

## Host test (tests/test_term_engine.c, test_kgr_a_redraw_beside_a_door_picture_does_not_repaint_it)
Door-shaped stream: 100x75 opaque RGBA stored a=t,c=20,r=10, placed at 8;25 with p=1,C=1. 5-plane screen, picker on, 12 free pens (as KG-32). Counted per redraw: text cells painted on picture cells (probes), picture runs blitted, picture pixels changed, and pixels written with a pen different from the one already there (new counter chunkyOff in the RectFill/WriteChunkyPixels fakes).
| redraw | text on picture | picture runs | pixels changed (final) | off-pen writes (in flight) |
|---|---|---|---|---|
| menu bar row 1 | 0 | 8 | 0 | 0 |
| border rows 22-24 | 0 | 0 | 0 | 0 |
| a=p again, same cell (scroll/layout re-place) | 0 | 204 | 0 | 0 |
Same result on 4 planes (247 runs for the re-place). Whole term_engine suite passes.

## Findings
- The 8 runs on a row-1 write are the cursor: C=1 leaves it on the picture's top-left cell (8;25); cursor hide/show redraws that one cell via VtCursorCell (vtbuiltin.c VtCursorCell -> VtDraw; term_cursor_paint disables VtRowCover, so the block is painted then the picture blitted over it). One cell, only while the cursor is visible and parked on a picture.
- A re-place of an unmoved picture re-blits all of it (engine makes a new serial per a=p; VtPicSlot -> VtPicMake re-picks pens), but pixel-identical, no intermediate pen. Costly, not visible on host.
- Nothing in DCTelnet paints background over the picture for these redraws. So with 1c23f50 the live flicker is NOT the text pass and not (a).

## Open (not verified)
The live stream differs from the modelled one. Candidates, in order: (1) the door's real blessed output does more than the headless capture (e.g. a full-screen repaint or ED that deletes/re-places pictures each render); (2) a visible cursor parked on a picture by C=1; (3) hardware-only: re-blit of 204 runs during beam visible as tearing although pens are identical.
Next step: capture the live wire bytes of one redraw from bbs.uprough.net (DCTelnet --capture or a host-side telnet log) and replay them through this test.
