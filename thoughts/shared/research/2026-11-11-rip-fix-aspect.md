# RIP aspect fix (int87, 2026-11-11)

Layer: vtcon engine decides size/placement (submodule commit 233e2331 on local/rip-pal-reset, on 77c3b2d); DCTelnet src/vtbuiltin.c VtImgRun only picks rows.
- rip_disp_h (rip_vt.c) = vt_aspect_rows(480, px_rx, px_ry); 350 when no pixel aspect set. vt_image.dh / vt_image_view.dh; vt_scale_row(r,h,dh) nearest row.
- Same map: tile rows (rip_reanchor, image_place_rows), vt_rip_take_dirty (mapped to shown rows, +-1 margin), rip_canvas_xy (click), host VtImgRun.
- vt_set_pixel_aspect flags the canvas moved; tiles are fixed at the next RIP flush.
- Test: tests/test_rip.c canvas_fits_the_pixel_aspect (aspect 8:15 -> 256 rows, 19 tiles, dirty, click). Fails 5 checks with rip_disp_h reverted to 350, passes after.
- Test run needed VTCON_NDK=/Users/spot/Code/UP-Term/vtcon/vendor/ndk-3.2r4-Include_H (the worktree's submodule lacks the NDK).
- Open: host VtImgRun scaled path is untested on a rig (not run); check the 8:15 value DCTelnet's VtSetAspect really sends for PAL hires.
