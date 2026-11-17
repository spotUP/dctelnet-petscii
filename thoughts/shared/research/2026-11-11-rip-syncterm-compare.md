---
date: 2026-11-11
topic: RIP rendering, SyncTERM ripper.c vs DCTelnet (colour, aspect, text after RIP, update cadence)
status: complete
---

Static reading only. SyncTERM = ripper.c (scratchpad/syncterm-src). Ours = worktree dctelnet-int87. Items marked UNVERIFIED were not proven by reading.

## A. Colour

SyncTERM:
- ripper.c:504-512 `curr_ega_palette[16] = {0,1,2,3,4,5,20,7,56..63}` (= default_ega_palette). `ega_colours[16] = {0x10,0x14,0x12,0x16,0x11,0x15,0x13,0x17,0x18,0x1c,...}`: RIP colour i draws with terminal palette slot 16+attr2palette(i), i.e. IBM attribute order (1=blue) converted to the terminal's SGR-order slots (slot 4=blue).
- set_ega_palette (7741): for i in 0..15 `attr2palette(i,&fg); ega_colours[i]=fg; setpalette(fg, ega_palette[curr_ega_palette[i]]...)`. It REPROGRAMS the terminal's own palette entries to the EGA colours. Drawing (map_rip_color 8362: `if (color<16) col = ega_colours[color]`) stores a palette slot, not an RGB.
- |Q (13930-13946) sets all 16 + set_ega_palette; |a (14235-14253) sets one slot + setpalette. Both change the live screen instantly.
- RIP_RESET_WINDOWS (`!|*`, 13348 -> reset func 8383-8438): `memcpy(curr_ega_palette, default_ega_palette); set_ega_palette();` (8422). Also at 19164 (RIP start/exit with ripver) and 12234 (reinit). `!|1K`/ESC[1! just toggles RIP mode; the palette is restored by the next `!|*` or on exit (normal_palette()).
- rip.color is NOT reset by `!|*` (8390).

Ours, traced end to end (index 0x0D and 0x01), all links CORRECT:
- rip_draw.c:487 `|c`: `g->color = a[0] & 15`; `|S` (494) `fcolor = a[1] & 15`; canvas pixels = palette index 0-15 (same as SyncTERM's index space).
- rip_pal.c:5-8 default master `{0,1,2,3,4,5,20,7,56..63}` identical to SyncTERM; rip_ega_rgb (rip_pal.c:14) is the standard rgbRGB decode. 0x0D -> master 61 -> R=0xAA+0x55=FF, G=0x55, B=0xFF -> 0xFF55FF. 0x01 -> master 1 -> 0x0000AA.
- rip_vt.c:113-115,303-304 copy cv->pal[i] (0xRRGGBB) into the image view pal; vtbuiltin.c:421-430 `vtImgMap[i] = term_pen_map[VtPenFor(VT_COLOR_RGB|pal[i])]`.
- vtpens.c:18-45 Vt_PenFor: nearest of prefs.AnsiColors (RGB4 *17). Default AnsiColors (palette.c:9) hold exactly A00/00A/.../F55/55F... so 0xFF55FF -> pen 13 (0x0F5F), 0x0000AA -> pen 4 (0x000A): exact. term_pen_map[n] = n unless ansiOwnPens (DCTelnet.c:458, 5714) maps ANSI pens to own pens on deep screens (retro32-term/term-engine.c:1087).
- No ANSI/EGA order swap exists in our chain: we never use slot numbers, only RGB -> nearest, so SyncTERM's attr2palette swap has no analogue and is not our bug.

Concrete difference (design, not a mis-index): we do NOT program the screen palette. We take the 16 EGA RGBs and snap each to the nearest of prefs.AnsiColors / ObtainBestPen. This is exact only if the screen's pens 0-15 are the default ANSI palette. Lossy/random-looking when: prefs.AnsiColors is a user/site palette (site_prefs.c:56,187 copy AnsiColors per site), the DeviceColors palette is active, DisplayDepth<4, or on 256-colour screens where VtPenFor(RGB) goes to ObtainBestPen (vtbuiltin.c:86-107), whose pens are shared with the Workbench and can be returned approximate when OBP_FailIfBad fails (-> falls back to Vt_PenFor nearest). I could NOT find a pure indexing bug by reading; UNVERIFIED which of these produces the on-screen mismatch. Needs one runtime check: dump vtImgMap[0..15] and the screen's LoadRGB4 table at the first RIP blit.

Also risks seen: (1) vtImgSerial cache (vtbuiltin.c:416) is keyed on a hash of the 16 RGBs only (VtImgKey 275), so it is stale after anything that changes pens but not the palette (prefs palette change, screen reopen resets it at 229/249/1533 - those look covered). (2) vtImgMap entries >= npal are never written (stale from an earlier picture); harmless for the 16-entry canvas.

Fix implied: do what SyncTERM does -- on RIP activation (and after |Q / |a / |*), program screen pens 0-15 to the EGA RGBs (SetRGB32 / LoadRGB4 on scr->ViewPort, pens = the ones term_pen_map[0..15] names) and make the RIP canvas index map identity-to-pens, saving/restoring the ANSI palette on RIP exit (SyncTERM shadow_palette()/normal_palette()). That removes nearest-colour guessing for the 16 colours entirely and makes |Q colour cycling cheap (a palette load, no re-blit). On 256-colour screens give the 16 RIP entries their own reserved pens (the vtTc slots) and SetRGB32 them rather than ObtainBestPen per run.

## B. Aspect

SyncTERM: canvas fixed 640x350 (ripper.c:345-348 defaults; 8395-8396, 8821 `rip.x_dim=640; rip.y_dim=350`). `rip.x_max = min(vstat.scrnwidth, 640)`, `rip.y_max = min(vstat.scrnheight, 350)` (8392-8400). Coordinates are scaled by xmap/ymap: `rip.ymap[i] = roundl(i * rip.y_max / rip.y_dim)` (8672-8673), with the source comment "TODO: Hack... we should likely scale both directions" (8404). Nothing is aspect-aware: on a 256-line screen it simply squeezes 350 lines into 256 (drawing is scaled, not blitted).

Ours: rip_vt.c has no px_rx/px_ry use (grep empty). Canvas is a 640x350 image (rip.h:79-80) shown 1:1 as 8-row tiles (VtImgRun, vtbuiltin.c:~660). The aspect fix exists only for kitty/sixel (kitty.c:1067-1078 vt_aspect_rows, vtbuiltin.c:1425-1442 VtSetAspect sets t->px_rx/px_ry). So 350 rows land on 350 PAL-hires pixel rows = about twice the intended height.

Fix: size the canvas placement, not the drawing: show 640x350 in 350*px_rx/px_ry rows (PAL hires 1:2 -> 175 rows, or fit to window height), by vertical row-decimation/averaging in VtImgWrite (source row = dy*350/rows) or by drawing into a canvas of CV_RIP_H = vt_aspect_rows(350,rx,ry) with the y-mapping done in rip_draw (SyncTERM style ymap). Row-decimation at blit time keeps one source of truth and is cheap on the 68k (pick source rows). Mouse mapping (rip_canvas_xy, rip_vt.c:621) must use the same map.

## C. Text after RIP

SyncTERM: the RIP screen is a graphics layer with a separate text window (rip.stw, `!|w`, 8537-8541, 13340-13347). Ordinary text and ESC sequences go through cterm into that text window, drawn over the graphics at the cursor; nothing clears the graphics unless `!|*` (13348: sets 80x43 text, cursor home, clears BOTH windows to the background colour, resets palette, drops mouse regions) or `!|E`/`!|e`. ESC[2J inside RIP mode only clears the text window region (cterm). A zero-size window `!|w 0 0 0 0` sets rip.text_disabled=true (8728, 14856) and every non-RIP byte, including ESC, is suppressed (18877, 18976) until a later `!|w`. Cursor: stw.x/y, restored from rip.stw at 8540-8548. So SyncTERM does NOT clear on a login prompt by itself; a board that wants a clean prompt sends `!|*` (or `!|1K`/`ESC[0!`+clear).

Ours: text goes to the vtcon grid; the RIP canvas is a z<0 image under the text in the same grid (rip_vt.c:505 "the text may clear the lines the tile is on: the canvas stays"). So text is drawn over the image at the grid cursor = top-left, exactly the symptom. Leaving RIP is heuristic: VtClsScan/VtRipAutoLeave (vtbuiltin.c:1619-1660) drops RIP only on ESC[2J with no RIP command after it in the same write, and only in Auto mode (RIP On stays on). `!|*` handler (rip_vt.c:171-179) clears grid and canvas.
Difference: the web terminal evidently clears on the prompt because the board sends a clear/!|* (or ESC[0!/ESC[1!); we only handle the clear if it arrives in a write with no later RIP command, and not in RIP On mode. UNVERIFIED what AmiExpress sends: capture the bytes after the last RIP command.
Fix: (1) on `ESC[2J` or `!|*` while RIP is active, clear the canvas AND the grid (SyncTERM clears both on `!|*`), do not require "no command in same write"; (2) honour ESC[1! / `!|1K` (v==1 branch, rip_vt.c:322) by dropping the canvas image, not only setting active=0; (3) if text arrives with RIP active and no text window set, place the text cursor in rip.stw (default full 80x43) instead of row 0 only if a `!|w` was received -- otherwise SyncTERM draws at the cursor too, so the prompt-over-image look is "correct" absent a clear; the real fix is (1)/(2).

## D. Update cadence

SyncTERM: drawing writes straight to its video memory/pixel buffer; `hold_update` (8047, 8383-8438, 12121-12242, 19200-19395) suppresses refresh around multi-step operations (reset, reinit, input waits) and the ciolib refresh thread pushes to the display on its own timer (rate-limited), not per command. A whole screenful of RIP commands in one buffer yields one or a few screen updates.

Ours: VtFeedRip (vtbuiltin.c:1660-1683) slices the stream at each '|' or LF and after EVERY finished RIP command calls VtRipFlush (735: vt_rip_take_dirty + vt_flush, blit of the touched cells) then VtDrawPause (gives layers back). That is the "builds up like on old machines" design and is the slowness: bbstitle.rip has hundreds of |S/|B commands, each costing a layer lock, a flush and a WriteChunkyPixels/RectFill pass.
Fix: coalesce. Flush once per VtWrite (the final VtRipFlush at 1683 already exists), or by timer (e.g. at most every 4-8 frames, ~50-150 ms) / when a command count threshold is hit, keeping the dirty-rect union (cv_rect_union) across commands; keep the per-command path only behind a "slow draw" preference. Keep VtDrawPause per batch so menus still open.

## EGA reference table (SyncTERM default palette, ripper.c:508 default_ega_palette -> ega_palette[64][3] 391-, 16-bit values /257 = 8-bit)

| idx | name          | master | RGB hex |
|-----|---------------|--------|---------|
| 0   | black         | 0      | #000000 |
| 1   | blue          | 1      | #0000AA |
| 2   | green         | 2      | #00AA00 |
| 3   | cyan          | 3      | #00AAAA |
| 4   | red           | 4      | #AA0000 |
| 5   | magenta       | 5      | #AA00AA |
| 6   | brown         | 20     | #AA5500 |
| 7   | light gray    | 7      | #AAAAAA |
| 8   | dark gray     | 56     | #555555 |
| 9   | light blue    | 57     | #5555FF |
| 10  | light green   | 58     | #55FF55 |
| 11  | light cyan    | 59     | #55FFFF |
| 12  | light red     | 60     | #FF5555 |
| 13  | light magenta | 61     | #FF55FF |
| 14  | yellow        | 62     | #FFFF55 |
| 15  | white         | 63     | #FFFFFF |

Note SyncTERM's own ega_palette[] entries for masters 0-7 are exact; masters 8-15 and 56-63 follow the rgbRGB rule (rip_ega_rgb in rip_pal.c matches it). Our default master array equals SyncTERM's.
