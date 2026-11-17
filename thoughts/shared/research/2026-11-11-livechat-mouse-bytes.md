# LiveChat door: bytes per SGR mouse report (2026-11-11)

Verdict: NOT the door. Zero kitty APCs, zero picture-row or text-over-picture writes per motion report. Flicker is engine/terminal side (DCTelnet) or caused by something other than door output.

Harness: copied openDoor() from board/Doors/livechat/tests/inline-pictures.test.ts (createApp with fake bbs, kittyGraphics=true, fake hub socket), shared 1 picture (png, 10 rows, placed at screen 8;25), then fed `ESC[<35;x;yM` via session.bbsSession.doorInputHandler (server.ts:~270 path), 100 ms settle, captured bbs.write + sendImage/placeImage/deleteImage. Script (not in repo): scratchpad/mouse.ts, run with board node_modules tsx. Repo untouched, no board connection.

Results, 16 reports (10 diagonal across log + picture cells x=20..65,y=4..22; 6 adjacent cells on row 8, inside picture columns):
- kitty APCs per report: 0 (a=t/a=p/a=d all absent; no sendImage/placeImage/deleteImage calls). Setup emitted exactly one a=t,i=1 and one a=p,i=1,p=1.
- bytes per report: 62-80 typical; 152 first; 252-264 when pointer crossed rows 22-24 (input box border restyle: CUP 22:1 22:12 23:1 23:74 24:1 -- focus/hover border, not picture rows).
- cursor moves: 2-4 CUPs per report, all on row 1 (menu bar animation text) except the border rows above. Row 1 writes also occur with NO mouse input (idle 200 ms: 127 bytes, rows 1 only), so they are a timer animation, not mouse-driven.
- Rows 8-17 (picture) / log text rewritten: never.
- Reports at (30,8),(35,10),(40,12),(45,14),(50,16) lie on picture cells: output 62-70 bytes, row 1 only.

Why: chat-pictures.ts layout() (features/chat-pictures.ts:~250-285) compares `where` to p.at and `continue`s when unmoved; confirmed by capture (no calls after render). blessed diff-renders only changed cells.

Caveats: headless blessed with fake bbs; wire bytes of `a=p` were synthesized by my stub of placeImage, so real encoding size is not measured (irrelevant, count is 0). Menu-bar animation timer (row 1) and border restyle on row 22-24 exist but are outside picture rows. Did not test dragging/button reports (only motion 35), nor scroll wheel (door scrolls log -> layout re-places with same placement id by design).

Proposed owner: DCTelnet (engine). Check what the engine does itself on each mouse report (redraw of picture cells / placement refresh in vtinput.c or the console flush) and whether row-1 animation writes next to pictures force a re-composite. Door needs no change for this symptom.
