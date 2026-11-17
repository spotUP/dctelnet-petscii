---
date: 2026-11-11
type: research
status: triage, read-only, nothing built or run
code: DCTelnet a093ca9, vtcon /Users/spot/Code/UP-Term/vtcon (971c3883), amiexpress-web board/Doors/livechat
---
# Image defects triage

## 1. IMAGE-MOUSE-FLICKER
Side: NOT DCTelnet. Door half probably already fixed in source; the rest is vtcon engine (unproven). Confidence: medium-low.

DCTelnet does no picture work per motion event:
- src/DCTelnet.c:4471-4484 IDCMP_MOUSEMOVE -> VtMouseMove (src/vtbuiltin.c:1402) + ToolBarHover (src/guis.c:2761, returns at once unless the tool bar wants hover). No redraw, no damage.
- src/vtinput.c:41-52 Vt_MouseMove sends a report only when the CELL changes (lastX/lastY) and ?1003/?1002 is on. Reports are bytes to the host only.
So DCTelnet's cost is: one SGR report per cell crossed. Not a repaint.

Door (read-only, owned by amiexpress-web-02): blessed enables ?1003h (sdk/engines/ui/blessed/core/program.ts:1060), so every cell crossing reaches the door, and blessed can hover-render.
- board/Doors/livechat/server.ts:1671 `screen.on('render', () => pictures.layout())`: layout runs after EVERY render, including hover renders.
- board/Doors/livechat/features/chat-pictures.ts:~250-285 layout(): `if (where === p.at) continue;` so an unmoved picture sends nothing; a moved one is re-placed with p=1 (PLACEMENT) and no a=d first (comment at :253-258). a=d only when the picture leaves view (:259). This is the peer's fix (ledger: "p=1 on every a=p, no a=d before move"). It matches the suspect "a=d then a=p" only for the OLD door build.
- Hover alone therefore should send no kitty traffic with the new door. If the rig-3 run used an older door build, suspect 1 (door a=d+a=p per render) is the cause and is already fixed.

vtcon engine: writing text over a placed picture does not remove it (engine/vtengine.c:5404-5425 under_mark; placements hang off the line, l->img, :460/:590 dropped only when the line is cleared/freed). Text rewritten by a hover render marks cells dirty, so picture rows under those cells are re-blitted (src/vtbuiltin.c:669 draws cells [a,b) of a row). Blessed writes only changed cells, so a hover over a widget that overlaps the picture rows would redraw only those cells. I did not find a path in vtcon that redraws pictures on pointer/mouse input itself.

Proposed fix / checks:
1. Confirm the rig-3 door build has the p=1/no-delete layout() (ask amiexpress-web-02 for the live commit; ledger says TS LiveChat has it, "LC 68K still no p=" is still open). If the flicker comes from the 68K LiveChat, the cause is that door sending a=d+a=p: fix on door side.
2. If it still flickers with the TS door: log the bytes the door sends on a mouse move (rig text log of the serial/telnet stream, no screenshot). Zero kitty APC bytes => engine/redraw side: then find which cells blessed rewrites per hover and whether vtcon re-blits the picture rows for them (a per-cell blit of unchanged-pixel picture cells is the only candidate); fix would be in vtcon to skip damage when a text cell is rewritten with identical content.
Owner input: which LiveChat (TS or 68K) and door build; whether moving over the picture itself or anywhere flickers.

## 2. IMAGE-ARTIFACTS (blue speech bubble)
Image not opened. Text description exists: ledger todos.md line ~443 and research/2026-11-10-kitty-picture-colours.md:55-75.
Description: flat blue now solid; faint 1 px cyan/teal dotted horizontal lines at regular spacing (~60 screen px at 2x), partial widths; outline continuous. Jagged outline = point-sampled enlarge of a small hub PNG.
Side: vtcon (engine/kitty.c) most likely; hub/door secondary. Confidence: low-medium.
- The earlier seam cause (dither of a dither, kit_sample box average re-dithered) is described at kitty-picture-colours.md:55-75 and was fixed for >255-colour pictures by vtcon 5fe845d6 (kit_adapt, 255-entry undithered palette, kexact). Solid blue shows that fix works.
- Residual lines are teal, not blue-on-blue: a different colour entering the scaled rows. Candidates, unverified: (a) kit_sample/kit_band reading the row beyond the strip edge at band boundaries, (b) an unset pen (index 0 / transparent) in the exact-colour path at band seams, (c) hub PNG anti-aliased edge row; the new hub q=pal path (<=255 colours at cols*8 x rows*16, no scaling) removes the resample, so it should disappear if (a)/(b).
Proposed: after the IMAGE-SCROLL cache lands, retest with the q=pal PNG sized exactly cols*8 x rows*16 (no scaling). If lines remain, they are in the whole-row crop path: check kit_sample's bounds at crop edges in engine/kitty.c with a synthetic flat-colour PNG (tests/, no rig). Do not fix before then (the scroll agent changes the same path).
Owner input: retest on the build with hub 3bd8284 live; say whether lines appear on a stationary picture or only after scroll/crop.

## 3. RIP-COLOUR-16
Side: DCTelnet (src/DCTelnet.c, src/vtbuiltin.c), possibly already fixed. Confidence: medium.
- The owner report (todos.md:501) was made on build 0a8cdcc/live 0c3d4ab25, BEFORE 6e7312f (ledger :516, rig 3 now c233283d). Retest of the new build is pending.
- Mapping now: src/vtbuiltin.c:437-456 VtImgColours: for a RIP canvas (VtIsRipCanvas :266-270: no kitty, fmt 0, z<0, npal==16) it calls VtRipShowColours(rgb) (src/DCTelnet.c:1940-1951); on TRUE vtImgMap[i]=term_pen_map[i] (colour N -> pen N, no nearest-colour search). SetPensRgb (:1896-1910) writes SetRGB32 on pens 0-15 (own-pens path: ansiColourPens). So a 16-colour fullscreen screen DOES take the exact load: gate is only `STATE_IS_NOT(APP_FULLSCREEN) || TerminalIsTrueColour()` (>8 bit RTG). On a Workbench window it falls back to the nearest-pen map (shared pens, never recoloured).
- Cost on 16 colours: pens 0-15 are the whole palette, so the toolbar/title/menus drawn with them recolour while RIP is up and return via LoadAnsiPalette (:1925) on exit; a 16-colour screen that is not fullscreen cannot do exact colour.
- Open: the separate ledger line "colours wrong on 256-colour screen too ... suspect EGA index order (1=blue) fed as ANSI order (1=red)". With 6e7312f index N is pen N holding the RIP RGB, so order is moot on fullscreen. If colours are still wrong, the palette itself (v->pal from vtcon rip_pal.c, 16 EGA RGBs) is the suspect, not the pen map. Not checked here.
Proposed: no code change until owner retests c233283d on the 16-colour fullscreen screen. If still off: dump v->pal[0..15] (vtcon engine/rip_pal.c) against the EGA table and print the pen RGBs actually set (VtRipShowColours) via the existing debug log.
Owner input: retest on c233283d; fullscreen or Workbench window; which colours wrong.
