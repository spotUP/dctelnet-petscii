---
date: 2026-11-10
topic: Why LiveChat inline kitty pictures scroll "weird and slow" in DCTelnet (vtcon)
status: research (read-only, nothing built or measured on hardware)
code: dctelnet-int87/src/third_party/vtcon @ working tree; amiexpress-web board/Doors/livechat (read-only)
---

# Kitty picture scrolling in the LiveChat door: cost and root cause

## What the door sends per scroll step

`board/Doors/livechat/features/chat-pictures.ts`
- Header comment l.6-10: "the log is repainted by blessed, not scrolled by the terminal, so layout() moves each
  placement after every render. A picture partly scrolled out shows the slice still in view (a kitty source crop)".
- `layout()` l.245-264, per picture whose `row:col:skip:shown` changed (i.e. EVERY scroll step):
  1. `bbs.deleteImage(p.id, true)` -> `ESC_G a=d,d=i,i=N` (placements gone, data kept)
     (`web/backend/src/utils/kitty-graphics.util.ts:150`).
  2. first time only: `a=t` store of the PNG.
  3. `a=p,i=N,c=<cols>,r=<shown>,x=0,y=<round(skip*per)>,w=<pxw>,h=<...>,C=1,q=2`
     (`kitty-graphics.util.ts:123-131`, crop from `sliceCrop` l.133-138).
- No `p=` placement id is sent, so the engine cannot treat it as "the same placement again".
- `sliceCrop`: `per = px.h / rows` is fractional in general; y and h are `Math.round`ed, so the source band
  per row jitters by +-1 source pixel from step to step while the target height `shown*ch` is exact.
- Picture size: `cols = logWidth - 2` (about 78), up to `INLINE_KITTY_MAX_ROWS = 64` rows (l.87, 91-95).

Order on the wire per step: blessed rewrites the log rows (text moves up one row) -> delete -> place.

## What vtcon does per a=p (engine)

`engine/kitty.c`
- `a=p` branch l.1384-1406: builds a `kit_src` over the STORED picture's indices (`s.bpp = 1`, `s.pal = im->pal`,
  the a=t picture was already quantised and dithered to the 6x6x6 cube + grey ramp by `kit_store` -> `kit_picture`
  l.834) and calls `kit_show`.
- `kit_show` l.722-806: computes tw = c*cw, th = r*ch, then `slot = kit_picture(t, s, x0, y0, w, h, tw, th)` (l.791):
  "crop baked per placement". There is no reuse path: every a=p makes a brand-new picture.
- `kit_picture` l.262-306 -> `kit_make` l.214-227 -> `kit_room` (may evict) + `VT_MALLOC(tw*th)` + `kit_index`
  l.191-211: for EVERY output pixel `kit_sample` (l.134-163: 4 long divides for the source rect, up to 4x4 = 16
  `kit_px` reads when shrinking) then `kit_quant` (l.100-114: 3x `kit_level`, each `v*n/255`, `v*n%255`,
  `rem*16/255`). With `image_copy` on (few-pen screen) it also builds a 3-byte RGB copy (l.276-290).
- `image_install` (vtengine.c:5470-5518): a free slot (`image_evict` when 16-ish slots are full),
  `im->serial = ++t->img_serial` (l.5511) -> a NEW serial every step.
- `image_place(t, slot, 2)` (vtengine.c:5527-5566, C=1 = mode 2, no scroll): `sixel_place_row` for each visible
  row -> `mark()` damage + `place_add` (l.~5600) which unlinks older same-layer placements the new one covers.
- The a=d,d=i before it (kitty.c ~l.960-1000) frees the previous kind-1 slot and marks its rows damaged.

## What the renderer does per step (render/amiga_render.c)

- New serial -> `img_colours` l.2774-2806 misses its one-entry cache (`r->img_serial == v->serial`, l.2780).
- On a palette screen (depth > 4, or rgb/exact) -> `pic_slot` l.2753-2771 misses (keyed by serial, only
  `VR_PIC_SLOTS 3`, amiga_render.h:32) -> `pic_release` of the LRU slot (ReleasePen of all its pens) +
  `pic_make` l.2641-2750: `pp_weights` over all w*h pixels, then ObtainBestPen (`vr_pen_best`) for each used
  colour (up to ~255), and when pens run out `pp_pick` + `pp_plan_make`. The weights differ per crop, so the pens
  picked and the dither plan can differ per step.
- Damage: the deleted rows and the new rows are redrawn (image rows blitted again in full, plus the text rows
  blessed rewrote).

## Cost estimate (unmeasured; 68020 at 14 MHz, A1200)

Per output pixel: ~10 32-bit divides (DIVS.L/DIVU.L ~ 44-90 cycles each on 68020) + kit_px + call/loop overhead
=> roughly 800-1500 cycles at 1:1; more when shrinking (up to 16 samples).
Typical picture: 78 cols x 8 px = 624 px wide; 10 visible rows x 8-16 px = 80-160 px high
=> 50k-100k pixels => ~50-150 M cycles => **about 3-10 s per scroll step** at 14 MHz (half on a 28 MHz 030).
Plus pic_make (one pass over all pixels + up to ~255 ObtainBestPen/ReleasePen) and the full redraw of the rows:
tens to a few hundred ms. Rescaling dominates by one to two orders of magnitude. All of it is thrown away on the
next step. (A cycle count on the rig / a `kit_placed` + timer counter would confirm; not done here.)

## The visible "weird" effect

1. **Picture lags the text**: blessed moves the text first, the old placement stays put over the wrong rows for
   the whole re-sample (seconds), so text visibly slides under/through the picture.
2. **Hole / flash**: the a=d,d=i blanks the old rows before the (slow) new picture exists: picture vanishes,
   then reappears one row higher.
3. **Shimmer / wobble**: every step re-samples an already-dithered picture (double dither: grey-ramp entries are
   not cube-exact, `g % 51 != 0`, so `kit_quant` re-dithers them), the ordered-dither phase is anchored at the new
   placement's (0,0) while the source moved by a fractional number of pixels, and `sliceCrop` rounding changes the
   scale ratio by +-1 px per step -> the picture's content jitters and its noise pattern crawls each step.
4. **Colour shift between steps** on palette screens: new serial -> new pen pick per crop (`pic_make` weights
   differ), LRU pens released, so the same picture can change colours as it scrolls.

## Root cause

The engine treats a crop as a new picture: `kit_show` bakes (scale + quantise + allocate + new serial) every
placement, instead of a placement being a VIEW on stored data, which is what kitty placements are. Everything
downstream (re-quantise, pen churn, renderer cache misses) follows from the new serial. The door's
delete-then-place and fractional crops make it visible but are not the cost.

## Fix (engine first, layer = vtcon engine kitty.c / vtengine.c; renderer needs no change)

The placement machinery already places rows of an image by index: `sixel_place_row(t, slot, y, x0, k)` /
`place_link(..., col0, row=k)` put image row k on screen row y, and `kit_place_at` already draws a band of rows
of an existing slot. So a row-band crop needs no new pixels:

1. **Scaled master per (stored id, tw_full, th_full)**: on a=p, compute the full-picture target size implied by
   the keys (tw = c*cw; th_full = round(s->h * th / h)). If an unscaled-x crop (`x0 == 0 && w == s->w`) maps to a
   whole-cell row offset (`skip_rows = round(y0 * th_full / s->h / ch)`), make/reuse ONE kind-1 picture of the
   whole stored image at (tw, th_full) — made once, same serial forever — and place only its rows
   `k = skip_rows .. skip_rows + r - 1` at the cursor rows (a variant of `kit_place_at` / `image_place` mode 2
   taking a first row k0). Rounding to whole cell rows also absorbs the door's +-1 px `sliceCrop` jitter.
2. **Keep the master alive across a=d,d=i**: it belongs to the stored picture (a pointer/serial on the kind-2
   slot, freed with it, evicted first by `kit_room` like a stored picture), not freed when refs hit 0 — otherwise
   the door's delete makes it re-scale every step anyway.
3. Arbitrary crops (x offset, non-row-aligned) keep today's bake path.

Effect: per step only `sixel_place_row` x visible rows + damage of those rows; same serial -> `img_colours`
/`pic_slot` hit, no pic_make, no pen churn, no re-dither shimmer. Cost goes from seconds to the row blits.
Regression test: drive `vt_write` with a=t then a=p crops at skip 0,1,2; assert `t->img_serial` / a
"kit_baked" counter rises once, and the cells' placement rows k step by one.

## Door side (amiexpress-web; owned by another session — advice, not required for the engine fix)

- Send a placement id (`p=1`) on every a=p and drop the preceding `a=d,d=i`: `kit_show` l.776-782 already frees
  the same id+p placement before installing the new one, and `place_add` replaces covered rows, so the move is one
  operation with no blank-hole frame.
- Cheapest pattern: let the terminal scroll. If blessed scrolled the log region (DECSTBM + IND/SU, smartCSR)
  instead of repainting it, vtcon placements live on lines and move with the text for free (no image traffic at
  all); only rows entering at the bottom/top need an a=p for the newly visible band. With engine fix 1 that band
  is a row-range place on the cached master.
- Size the PNG to an exact multiple of the row height (`px.h` divisible by `rows`) so `sliceCrop` returns integral
  per-row bands (removes the +-1 px jitter even on terminals that bake crops).
- Note: C=1 (mode 2) cuts at the SCREEN's last row, not the scroll region's (vtengine.c:5540); the door's crop to
  `shown` rows is what keeps a picture off the input/status lines below the log. Keep cropping.

## Fix (2026-11-10, vtcon branch fix/mouse-any-personality)

Implemented in the engine only (renderer unchanged):

- `kit_band` (engine/kitty.c): an `a=p` of a stored, non-animated picture whose crop is the whole width
  (`x=0`, `w` all), has `c=` and `r=`, no `X=`/`Y=`, no parent, not virtual, and is not cut at the right edge or the
  height limit, is a row band when `y0*r/h` is within a quarter row of a whole row k0. The whole picture is then
  scaled ONCE to `tw x rows*ch` (rows = round(s->h*r/h)) through the same `kit_picture` (exact <=255-colour path,
  cube dither, RGB copy, RGBA format all unchanged) and kept as a kind-1 slot flagged `kmaster`. The placement is its
  rows k0..k0+r-1 via `image_place_rows` (image_place with a first row and count; image_place calls it with 0, all).
  `krow0` keeps kit_corner's top-left right for relative children.
- Lifetime: `kit_drop` -> `kit_unplace` for a master on `a=d` (d=i/a/p-replace in kit_show): cells marked, gen++,
  refs 0, children freed; pixels and serial kept. `place_release` no longer frees a master when its last row leaves.
  `image_free` of a kind-2 stored picture frees its master (d=I, re-store, eviction). One size per id: a master of
  another size with no placement is freed; one still on the screen as another placement makes the new crop bake.
- `kit_placement` skips an off-screen master (it is not a placement then).
- Sentinel: `t->kit_made` (every kit_picture), `vt_kitty_pictures_made()`.
- Other crops (x offset, partial width, half-row y, no c/r, animated pictures) keep the bake path.

### Fix: results
- vt_free now frees all images before the slot structs; the kmaster cascade exposed an ASan heap-use-after-free in image_free when vt_free was called.
- Tests after fix: kitty 521/0 failed, render 775/0, sixel 451/0. With the fast path disabled, kitty fails 6 checks (test_kitty.c 1451, 1456, 1460, 1461, 1463, 1467).
- dctelnet-int87 fbe0463 (not pushed), host-ci rc 0. Installed into UP-Term/vtcon/build/rig3/vtc/dct/DCTelnet.020, md5 da22fb24a11d71af5775c28e2250934e (backup DCTelnet.020.643ff688, md5 9231f0dde046206e60ca7d2786d9fd13).
- Open: not measured on a rig; renderer cache hit with the same serial unverified; kit_delete_where (position/z deletes) still frees the copy; animated pictures keep the bake path.
