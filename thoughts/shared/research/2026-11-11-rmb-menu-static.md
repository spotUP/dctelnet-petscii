# RMB menu corruption, static review dc29c0a..7c08df3 (src/ only)

Method: git diff of src/, read lock code at 7c08df3 (VtLayersLock 934, VtDrawBegin/Pause/End 969-998, VtDamage 1000). Not run.

## (b) colour / geometry: NO suspect at 4 planes
- No SetRGB*/LoadRGB*/ScrollVPort/MakeScreen/RethinkDisplay/MoveScreen/ChangeScreenBuffer/RxOffset added anywhere in src/ diff.
- Only palette-touching change: ObtainBestPen/ReleasePen (vtTrueColour was depth>8, now Vt_ObtainsPens: depth>4, vtpens.c). Gated by depth<=4 -> FALSE, so a 4-plane screen never reaches it (VtOpen ~1466, vtPicPens needs depth 5..7).
- DCTelnet.c diff: only VtReopenPending, VtHeldResume call, replay timing. No screen/mouse/IDCMP changes (195299c is a vtcon submodule bump only; engine-side).
- retro32-term submodule unchanged in range (term_* host layer identical). vtcon bump 6d3df26..f9fa052 is outside src/ diff: unreviewed here.

## (a) unlocked bitmap writes
1. SUSPECT, medium: held-tail loop in VtWriteIn (src/vtbuiltin.c:1754-1769), commit 3a0da7a/0f1cef4 family (RIP Auto "rest of chunk").
   vtInWrite = FALSE (1754) is set BEFORE the new `while (vtHeldLen && !VtReopenPending())` loop, which calls VtFeedRip -> vt_feed/VtRipFlush -> vt_flush (738) -> VtDamage (1000). VtDamage only calls VtDrawBegin when vtInWrite, so VtDraw/VtDrawImages/VtImgWrite run with no LayerInfo lock and term_covered stale from the last lock (FALSE = direct plane writes). VtDrawPause (1680) then makes it worse between commands. VtParseEnd (1771) only draws after. Same hole via VtHeldResume (1784) -> VtWriteIn(NULL,0): VtFeed returns early on len 0, loop at 1760 draws unlocked after reopen. Fix would be: keep vtInWrite TRUE until after the held loop. Only reachable when RIP Auto (mode 2) turns active mid-chunk.
2. WEAK, low: VtFeedRip (1650-1684) VtDrawPause leaves term_covered stale-FALSE and cursor hidden while vtInWrite stays TRUE; any non-VtDamage draw in that window (cursor blink timer in DCTelnet.c main loop, pre-existing) would write planes unlocked. No new timer/animation draw path found in diff (grep: no kitty animation tick, no new cursor_show/hide).
3. Ruled out: new picture draws (RectFill 628, WritePixelArray8 663, VtImgBlit) go via VtImgRp()=term_rp or term_win_rp, i.e. window RastPort, layer-clipped, safe even unlocked. VtDraw/VtRedraw/VtScrollView/VtCursorCell bodies: no new unlocked callers.

## Conclusion
Nothing in src/ changes colours or screen offset at 4 planes. If the symptom reproduces on plain (non-RIP) text, the cause is not in the src/ diff: look at the vtcon/retro32 build (stale objects, Makefile DEPS/flags change in src/Makefile, vtcon bump) or at a different binary mix. Suspect 1 is the only lock-logic regression found, and needs RIP Auto.
