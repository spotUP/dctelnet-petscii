## What

- **RIP: draw a scaled canvas one cell row at a time** (ae00963). VtImgWriteRows in `src/vtbuiltin.c` writes a scaled RIP canvas one cell row per VtImgWrite instead of one pixel row. On a full-canvas redraw, runs drop from 248 to 31; the frame hash is unchanged.
- **Skip the text pass under cells an opaque kitty picture covers whole** (1c23f50). VtRowCover/VtCellCovered skip text cells that lie wholly inside an opaque kitty z>=0 picture, so a redraw no longer paints background over the picture before re-blitting it. Text cells painted over the picture per scroll step drop from 100 to 0.
- **Regression test for redraws beside a door-placed picture** (ba0f6c6). It replays the LiveChat door's kitty parameters and checks that a menu-bar write, a border restyle and a repeated placement change no picture pixels.

## Why

- The owner found RIP screens slow on 68k.
- The owner saw black bands over kitty pictures in the LiveChat door while it scrolled.

## Testing

- Host: the `tests/test_term_engine` suite passes, including test_rip_slow_2_a_scaled_canvas_is_blitted_in_batches, test_kgs_a_scrolling_opaque_picture_is_not_painted_over_its_text_cells and test_kgr_a_redraw_beside_a_door_picture_does_not_repaint_it.
- fs-uae (A1200 rig, 68020 build 1c23f50), live on bbs.uprough.net:
  - RIP speed is acceptable.
  - RIP colours on a 16-colour screen match the web viewer.
- The remaining LiveChat flicker, the scroll bar jitter and the sidebar scrolling with the log are caused by the door. DCTelnet has no left/right margins (DECSLRM). They are being fixed on the board side.

Research: `thoughts/shared/research/2026-11-11-rip-slow-2.md`, `2026-11-11-kitty-scroll-bands.md`, `2026-11-11-kitty-redraw-flicker.md`.
