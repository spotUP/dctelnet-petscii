---
date: 2026-11-11
topic: IMAGE-MOUSE-FLICKER root cause hunt (LiveChat door, kitty picture flickers on pointer move)
status: inconclusive -- root cause NOT found; terminal-side per-move path cleared by code reading
code: DCTelnet a093ca9, vtcon 5fe845d6, retro32-term b77fc5d3
---

# Kitty picture flicker on mouse move: root cause hunt

## Verdict

No file:line found. By code reading, a pointer move makes neither DCTelnet nor
vtcon draw anything over the picture rows, and the door bytes a move causes
touch rows 1 and 22-24 only, which the engine flushes row-exact. The
"door sends nothing to picture rows" + "terminal draws nothing on a move"
pair cannot both hold against the owner's report, so one of the capture's
assumptions does not match the live session (see "Open hypotheses").
No host repro was run (budget spent on the read; nothing to reproduce on the
paths read).

## Violated invariant (the one to protect)

Picture cells (rows 8-17, col 25+) must be redrawn only when the engine damages
those rows. Any redraw of them is visible flicker, because VtDraw
(src/vtbuiltin.c:861-948) draws the text cells (blank glyphs) first and then
VtDrawImages blits the picture over them; only an opaque RIP canvas skips the
text pass (VtRowOnCanvas, :872/:883). A kitty picture (z >= 0, VT_CELL_IMAGE)
always gets text-then-picture.

## Terminal-side per-move path, cleared (a093ca9)

- src/DCTelnet.c:4471-4484 IDCMP_MOUSEMOVE: VtMouseMove when the program owns
  the mouse, else SelectionMove; then ToolBarHover.
- src/vtbuiltin.c:1402-1411 VtMouseMove -> Vt_MouseMove (src/vtinput.c:41-52,
  takes `const vt_term *`, encodes only) -> VtReply (:1075) -> VtDrawPause
  (:1037, no-op outside a write: vtDrawOpen FALSE) + TCPSend (DCTelnet.c:595,
  socket send + ScheduleKeepAlive; no draw).
- SelectionMove (DCTelnet.c:4293) returns at once unless selDragging.
- ToolBarHover (src/guis.c:2761) draws only into toolBarWin's RastPort.
- Main loop after GetWindowMsg (DCTelnet.c:3265-3276): VtFlushHeld only, a
  no-op unless a UTF-8 prefix is held (vtbuiltin.c:1937).
- No scrollback reset: vtScrollOff only changes by the wheel/scrollbar.

## Drawing of the door's per-move bytes, cleared

- Re-ran scratchpad/mouse.ts (unchanged): per report 62-80 bytes, CUPs on row 1
  only (`ESC[1;46H ESC[0;48;5;204;38;5;204m /` pairs), 252-264 bytes when the
  pointer crosses rows 22-24 (border CUPs 22:1, 22:12, 23:1, 23:74, 24:1). No
  ?25, no ?100x, no APC, no row 8-17 write. Idle (200 ms): 127 bytes, row 1.
- vtcon engine/vtengine.c:904-934 flush(): one damage call per run of rows with
  equal spans, per-row dx0/dx1 -- rows 1 and 22-24 never widen to rows 8-17.
- VtDrawImages (vtbuiltin.c:803) loops only y0..y1 of the damage and clips x.
- Cursor hide/show (retro32-term term-engine.c:507-541) redraws only the one
  cursor cell (VtCursorCell -> VtDraw 1x1); the cursor ends on row 1 after
  each report.
- Pens: colour 204 is cached in vtTc after the first use (VtPenFor :85); no
  per-write pen change. VtPicSweep only acts when vt_image_frees changes.

## Open hypotheses (ordered), with the check that decides each

1. The live wire differs from the capture's input. The capture fed SGR
   `ESC[<35;x;yM` straight into doorInputHandler. The door itself enables no
   mouse mode (no ?100x in its output): the board/DoorInputManager does. If the
   live session has ?1003 without ?1006, DCTelnet sends X10 `ESC[M` + 3 raw
   bytes (32+x, 32+y; >127 past col/row 95). If the board or blessed misparses
   that (e.g. as ESC key + text, or a resize/refocus), the door re-renders the
   log box -> rows 8-17 rewritten -> engine keeps the placement
   (vtengine.c under_mark ~5404) -> DCTelnet redraws text then picture =
   flicker, on mouse move only.
   Check: log the bytes DCTelnet receives live during one pointer move over
   the picture (DCTelnet capture/replay log, or board-side wire log); count
   CUPs on rows 8-17. Also check DECSET ?1006 in the live stream.
   Fix layer if confirmed: board/door (mouse mode set or report parse), not
   DCTelnet.
2. The real door's hover handling differs from headless blessed (hover on
   chat log lines / picture element re-renders the log, or the door re-places
   the picture on hover). Same check as 1 decides it.
3. The flicker is the pointer, not the picture: RTG software pointer while
   DCTelnet draws straight into the screen bitmap with LayerInfo held
   (VtLayersLock, vtbuiltin.c:995) -- the row-1 writes at report rate.
   Check: ask the owner native chipset vs RTG, and whether the flicker is
   at the pointer only or the whole picture.

## Proposed next step

One live capture of DCTelnet's received bytes during a pointer move over the
picture (hypotheses 1/2), and one question to the owner (hypothesis 3). If
picture-row writes show up, the fix is in the board/door; in DCTelnet the
only mitigation would be skipping the text pass under an opaque kitty picture
(like VtRowOnCanvas does for RIP canvases), which cures the symptom for any
rewrite under a picture but is not the root cause.

## Confidence

High that the DCTelnet/vtcon per-move path itself draws nothing over the
picture (every function on it read). Low on which open hypothesis holds.
