---
date: 2026-11-10
topic: kitty picture colour quality (vtcon engine, DCTelnet 256 colours)
tree: /Users/spot/Code/DCTelnet/dctelnet-int87/src/third_party/vtcon (fix/mouse-any-personality, PR spotUP/UP-Term#13)
---

# Kitty picture colours: exact palette, one dither

## Owner report (rig 3, 256 colours, LiveChat door)
(a) flat white+blue picture shows dither; (b) banding in the dither; (c) "dont dither twice".

## Diagnosis (verified in code)
1. `engine/kitty.c` `kit_index` cut every index picture to the fixed cube (kit_color: 216 cube + 38 greys) with a
   4x4 Bayer (`kit_quant`). A flat colour off the cube (0x2050C8) became a dither pattern on every screen,
   cgx LUT included. Confirmed: symptom (a).
2. 16-pen screens (`r->few`): `img_colours` skipped `pic_slot` and put every cube entry on its nearest pen
   (`img_pen_for`): quantised twice. 5-7 plane screens already dithered once from the RGB copy.
3. `kit_picture` built the RGB copy by point sampling (not `kit_sample`): a shrunk photo aliased in the copy.
4. `kit_root_map` looked palette entries up as cube colours: an exact (off-cube) entry mapped to 0, so the root's
   pixels would turn clear when its animation frames started.
5. Found: `kit_anim_show` swapped a placement's pixels to the frame's cube indices but kept its RGB copy (the
   root's colours): the renderer's rgb mode would draw the root, not the frame. Copy now dropped there.

## Decisions / what changed
- `kit_index`: one pass with a 512-slot hash of colours; 255 or fewer (after kit_sample; alpha < 128 = index 0)
  give the exact palette, no dither, returns 1. The 256th colour restarts on the old cube path (one dither).
  New `vt_image.kexact` / `vt_image_view.exact`.
- The RGB copy (only where the renderer asks: palette screens below 8 planes, no cgx) is made for exact and
  dithered pictures alike, now from `kit_sample`. A first try without it for exact pictures made K32-PENS worse
  (bar error 30 vs 5): the renderer's own-colour path beats planning palette entries on 5-7 planes.
- Animation: frames stay cube canvases (unchanged design). `kit_root_canvas` dithers off-cube entries of an exact
  root to the cube per pixel; `kit_anim_show` switches on `kexact || npal != 256` (an exact palette of 255 colours
  has npal 256) and drops the placement's RGB copy.
- Renderer (`render/amiga_render.c`): `vr_pic_copy` on for every palette screen below 8 planes without cgx
  (16 pens included). `img_colours` runs `pic_slot` on 16 pens when the picture has its own colours (`v->rgb` or
  `v->exact`); a cube-dithered picture without copy (copy out of memory) keeps nearest pens (no second dither).
  `pic_make` candidate loop takes the terminal's pens on `r->few` under VT_AMIGA too (no free pens there).
- Result per screen: cgx 8-bit LUT: exact for <=255 colours, else cube dither (one). AGA 8-bit: exact pens per
  entry while free pens last; few-colour pictures exact. 5-7 planes and 16 pens: one dither from own colours.

## Open
- 16 pens, flat 0x6040A0 with no free pens: mean tile error x100 is 4950 with the own-colour mix vs 3408 before
  (nearest pen per cube entry). Gradient 2547 vs 2586. Not asserted in the test; cause in pp_bin_fill/pm mix not
  examined. Does not affect rig 3 (8 planes).
- AGA 8-bit with a >255-colour picture whose cube entries outrun the free pens: pic_make still mixes cube entries
  (second dither). The rgb path there is capped at PM_MAXPENS (32) pens, worse than ~240 exact pens, so the copy
  stays off at depth 8. Fix would need pen_match.h palettes beyond 32 pens.
- Banding for >255 colours on cgx 8-bit: still the 6-level cube with 4x4 Bayer. An adaptive palette (median cut
  from the histogram, pp_pick-style, 255 entries) plus dither to an arbitrary palette needs a per-pixel nearest-two
  search (pm_match is built for <=32 pens); not done. Owner call whether worth the 68020 cost.
- Animated pictures: frames are cube canvases, so an exact root shows dithered once frames play.

## Logo dither and seams (2026-11-11)

Owner report (rig 3, 8-plane AGA, no graphics card): the LiveChat chat-bubble logo's flat blue (#3a70d8) shows fine
vertical stripes of two blues, plus faint 1 px horizontal lines across the bubble about 53 screen px apart.

Cause, both defects (one root): the logo has more than 255 colours (anti-aliased edges, shading), so kit_index fell
back to the 6x6x6 cube with the 4x4 Bayer dither (kit_quant). On 8 planes DCTelnet has no pic-pen path
(vtbuiltin.c:1472, vtPicPens needs depth < 8), so each engine palette entry is one pen and the engine dither is what
the screen shows.
- Stripes: #3a70d8 under kit_quant raises pixels only on Bayer rows 0 and 2 (columns x = 0 and 2 mod 4); on AGA's
  tall pixels that reads as vertical stripes of two cube blues.
- Seams: a stored picture (a=t) was dithered once at its full size, then a=p / the scroll band (kit_band) resamples
  that dithered index image (kit_sample box average) and dithers the averages again. The average of a dithered flat
  area depends on how the box lines up with the Bayer period, so it beats against the scale: simulated
  (scratchpad seamsim.py, 256 source rows to 192-240 rows) the averaged flat blue takes 3-5 values and slips phase
  every ~15-16 output rows at 240/256; a scale near 1.04 puts the slip about 53 px apart, the reported spacing.
  Simulation, not a rig measurement.

Fix (vtcon engine/kitty.c, kit_adapt): a picture of more than 255 colours gets 255 entries cut from its own colours
(weighted median cut over a colour table of up to 2048 colours, coarsened a bit per channel when full; a box's colour
is its heaviest member when that holds half the box, so a flat area keeps its colour exactly), every pixel mapped to
its nearest entry, undithered. kit_index returns 1 (kexact): kit_anim_show still moves an animated picture to the cube
on its first frame, and kit_root_canvas dithers off-cube entries per pixel as before. The cube path stays only for a
failed allocation (~77 KB, freed at once). Frames (kit_blit, frame canvases) stay on the cube.

Tests (tests/test_kitty.c): a_flat_logo_with_antialiased_edges_stays_solid (320x256 logo, > 255 colours, copy off and
on: every flat body pixel is exactly #3a70d8, one index) and scrolled_picture_has_no_seam_rows (10 scroll steps, band
and bake paths: adjacent flat rows equal). With kit_adapt disabled: kitty 6 checks fail (off = 11777, idxs = 11714 for
both copy settings, seams = 615, the k38 exact flag), render 6 (K37 golden). Regoldened: K37 scenes 0/1 (the 4096-colour
gradient), K32 e_on < e_off relaxed to within 5% (own-colours copy 2032 vs cut palette 1986; was the cube), k38 exact
0 -> 1. Comments at each say why.

Open: rig check of the logo on rig 3 (stripes and seams gone; a slight colour shift on gradients where 255 entries are
fewer than the source). K32: the cut palette now beats the RGB copy path on 12 pens; owner call whether the copy path
is still worth its 3 bytes a pixel.

### The 89-colour PNG on rig 3 (2026-11-11)

The owner's dither on rig 3 is not a >255-colour source. Installed binary md5 da22fb24... is DCTelnet build fbe0463, whose vtcon is 465ec47c (has 643ff688). Host probe (scratchpad pngprobe.c, real hub PNG f33df3432e7c9.png, 424x432, 89-entry PLTE):

- square pixels: exact=1, npal=90, the blue #2675e2 on 93505 px, one index. The engine is clean here.
- pixel aspect 22:44 (PAL hires, what DCTelnet's VtSetAspect passes from the mode's DisplayInfo Resolution): the engine scales the picture to 424x216. The vertical box average mixes neighbouring palette colours, the result has more than 255 colours, and 465ec47c falls to the cube dither: exact=0, npal=32, 0 pixels keep #2675e2. That is the dither.
- same with kit_adapt: exact=1, npal=256, #2675e2 on 47552 px, one index.

Root fix: the engine's many-colour branch (kit_adapt, median cut to 255 entries, undithered), not DCTelnet. Rig 3's screen mode is assumed non-square (hires non-laced); unverified on the rig.

## 16-pen text colour loss

Read-only investigation, 2026-11-11 (repo dctelnet-int87, vtcon at 465ec47c). Unverified on the rig.

### Which code draws the picture -- decides everything

- DCTelnet ROUTE_BUILTIN (vtbuiltin.c) does NOT use render/amiga_render.c. At depth 4 `vtTrueColour` is FALSE
  (vtpens.c:54 Vt_ObtainsPens: depth<=4 -> FALSE), `vtPicPens` FALSE (vtbuiltin.c:1472), so VtImgColours
  (vtbuiltin.c ~430) maps every palette entry to the nearest of the 16 ANSI pens (vtpens.c:18 Vt_PenFor). No
  ObtainBestPen, no recolour. 643ff688's amiga_render.c changes cannot reach this route.
- Only the console route with vtcon's console.device (device/upcon_unit.c:279: personality VT_AMIGA) or a vtcon CON:
  window (handler/vtcon_handler.c:1858) runs amiga_render.c, i.e. the 643ff688 changes.

### Mechanism (amiga_render route)

1. VT_AMIGA text draws SGR 30-37/90-97 straight in screen pens 0-15 (pen_for, amiga_render.c:390-391,
   vr_screen_pen glyph_pen.h:10) and never obtains them.
2. Since 643ff688, a kitty picture on a few-pen screen enters pic_slot/pic_make (img_colours, amiga_render.c:2798;
   vr_pic_copy gives every depth<8 screen an RGB copy, :515 -> rgbmode). pic_make picks the heaviest picture
   colours (pp_pick) and asks `vr_pen_best(cm, rgb, 1)` for each (amiga_render.c:2695, :2707; exact loop :2671).
3. vr_pen_best = ObtainBestPen PRECISION_EXACT + FailIfBad (pens_amiga.c:10-23): with no exact match it takes any
   colour-map entry nobody holds and LOADS THE PICTURE'S COLOUR into it.
4. If text pens 0-15 are not held by anyone, the picture's blue/white goes into a pen the text is drawn in: that
   text changes colour on screen at once, and vanishes where the new colour equals the background.
   Which pens are unheld: DCTelnet holds 0-15 PEN_EXCLUSIVE only in fullscreen on graphics V39+
   (DCTelnet.c:6106-6121). Windowed on a 16-colour Workbench, Workbench holds only its own few pens; the rest are
   free -> stolen.

Caveat against "did not happen before": the pre-643ff688 few-pen path (img_pen_for, amiga_render.c:2613-2632)
also called obtain() = exact first (pen_rule.h:16-19) for every one of the 240 cube entries, so it could steal
free entries too (with dark cube colours). 643ff688 changed WHICH colours land in the stolen pens (the picture's
heaviest real colours: its blue and white, now dead on the text's colours), which is what makes text visibly vanish.
Fullscreen V39 with all 16 pens held exclusive: every exact obtain fails (got=0), the picture is dithered from the 16
terminal pens only, and no text pen can change. So the report implies a screen where pens 0-15 are NOT all held.

### Deciding measurement (one)

On the failing setup, before and after the picture: GetRGB32(cm, 0, 16) for pens 0-15 (or log pic_make's
s->held[] / n_pic_picks). A text pen's RGB changes -> mechanism above confirmed. No change -> the route is not
amiga_render (check Renderer_Route: builtin vs console) or the loss is not a pen recolour, and this note is wrong.
Also record fullscreen vs windowed and the renderer route of the rig run.

### Fix (root level)

Violated invariant: a renderer must hold every colour-map entry it draws with, so ObtainBestPen (its own or any
other program's) can never recolour a pen with live pixels. VT_AMIGA (and the few-pen terminal pens generally) draw
with pens 0..min(16, 2^depth)-1 without holding them.
Fix in render/amiga_render.c: when the screen is a palette screen and the personality draws screen pens (VT_AMIGA)
-- and on r->few for the candidate terminal pens pic_make already dithers onto -- hold those pens shared with
`ObtainPen(cm, n, 0,0,0, PEN_NO_SETCOLOR)` in vr_init / on a personality switch, release in vr_free. A pen already
held exclusive by the host (DCTelnet fullscreen) fails the obtain harmlessly: nobody else can take it either.
Not a fix: forbidding exact obtains on few screens (the picture would lose free pens legitimately available).

### Regression test (fits tests/test_render.c, fake map in tests/render_host/gfx_fake.c)

gfx_fake.c already models free entries: npal/fpal palette, pool_first/pool_n/pool_ref free pool; ObtainBestPenA
exact loads `fpal[i] = want` into a free pool entry (gfx_fake.c ~760-770). ObtainPen (gfx_fake.c:723) does not mark
pool_ref yet: it must set pool_ref[n] for a specific n (so the fixed code's hold is visible), else the test cannot pass.
Sketch, "kitty picture on 16 pens keeps the text's colours":
  planar_depth = 4; npal = 16; fpal[] = the 16 ANSI colours; pool_first = 4, pool_n = 12 (unheld, as on a WB);
  VT_AMIGA personality; write "\e[34mBLUE\e[37mWHITE" then a 2-colour kitty picture (0x2040C0 / 0xF0F0F0 pixels);
  paint; assert fpal[4..15] unchanged and n_pic_picks == 1 (sentinel: pic_make ran).
  Fails before (pic_make loads the picture's colours into pool entries 4.. -> fpal[4] == 0x2040C0), passes after.
