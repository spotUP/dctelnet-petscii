# Kitty picture scroll: black bands (IMAGE-SCROLL-BANDS)

Cause: confirmed on host. VtDraw (src/vtbuiltin.c) painted the text cells of every damaged row (black background), then VtDrawImages blitted the picture over them. The door repaints the log and does a=d + a=p per scroll step, so all picture rows are damaged each step.

Host repro: tests/test_term_engine.c test_kgs_a_scrolling_opaque_picture_is_not_painted_over_its_text_cells (80x80 opaque picture, c=10 r=10 = 100 cells, 6 door steps: repaint 10 rows, a=d,d=i, a=p).
Per step, before -> after:
- text cells painted on picture pixels: 100 -> 0 (6400 px of black no longer drawn then overdrawn)
- picture draws: 16 chunky blits + 80 flat fills -> unchanged (same picture, same area)
- glyph writes: 223-230 -> 123-130 (text beside the picture still drawn)
Final frame: picture area all one red pen (asserted); text beside it unchanged.
Test fails without the fix (assert text-on-picture == 0), passes with it; whole term_engine suite passes.

Fix: VtRowCover/VtCellCovered: a cell is skipped in the text pass only if it is VT_CELL_IMAGE in a kitty fmt-0 picture with z >= 0, lies wholly inside the picture pixels, and has no transparent (index 0) pixel. Edge cells, alpha, z < 0, RIP/sixel, cursor paint keep the old order. Cost: a byte scan of the covered cells per damaged row (unmeasured on hardware; early exit on first transparent pixel).

Commit: 755d011 on branch fix/kitty-scroll-no-black-bands (worktree /Users/spot/Code/DCTelnet/dctelnet-scrollbands), not pushed. vtcon untouched (pin 5fe845d6).
Binary: /Users/spot/Code/DCTelnet/dctelnet-scrollbands/build/vbcc-68020-release/DCTelnet.68020 (host-ci; docker was down, make host-ci used; BUILD_HASH shows a093ca9). md5 ae79b7e93dd3e9be909646008f16e2b3. Not run on a rig.

Sluggishness: this fix removes only the black text fill (100 cells x 8 rows per step). The per-step re-sample in vtcon (kitty-scroll-cost.md) is separate and untouched.

Scroll bar verdict (code reading): DCTelnet's only scroll bar is the BOOPSI Scroller (guis.c:1985, DCTelnet.c:4509) in the separate scrollback window; the built-in renderer's scrollback viewport (vtScrollOff, Shift+PageUp) has no gadget, and vtScrollOff is 0 during live output. So a bar jumping up and down in the LiveChat door is the door's own drawn bar (text repainted per step), not DCTelnet.
